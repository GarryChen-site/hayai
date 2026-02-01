
#include "hayai/net/TcpConnection.h"
#include "hayai/net/Channel.h"
#include "hayai/net/EventLoop.h"
#include "hayai/net/Socket.h"
#include <cassert>
#include <cerrno>
#include <unistd.h>

namespace hayai {
TcpConnection::TcpConnection(EventLoop *loop, int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(loop), socket_(std::make_unique<Socket>(sockfd)),
      channel_(std::make_unique<Channel>(loop, sockfd)), localAddr_(localAddr),
      peerAddr_(peerAddr) {

  channel_->setReadCallback([this] { handleRead(); });
  channel_->setWriteCallback([this] { handleWrite(); });
  channel_->setCloseCallback([this] { handleClose(); });
  channel_->setErrorCallback([this] { handleError(); });
}

TcpConnection::~TcpConnection() { assert(state_ == State::Disconnected); }

void TcpConnection::send(std::string_view message) {
  if (state_ == State::Connected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      loop_->runInLoop([this, msg = std::string(message)] { sendInLoop(msg); });
    }
  }
}

void TcpConnection::sendInLoop(std::string_view message) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = message.size();
  bool faultError = false;

  // If output buffer is empty and not writing,
  // try to write directly to avoid Poller overhead
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->fd(), message.data(), message.size());
    if (nwrote >= 0) {
      remaining = message.size() - nwrote;
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        if (errno == EPIPE || errno == ECONNRESET) {
          faultError = true;
        }
      }
    }
  }

  // if we couldn't write all data, buffer the rest
  if (!faultError && remaining > 0) {
    outputBuffer_.append(message.data() + nwrote, remaining);
    if (!channel_->isWriting()) {
      // Start monitoring for writable events
      channel_->enableWriting();
    }
  }
}

void TcpConnection::handleRead() {
  loop_->assertInLoopThread();
  int savedErrno = errno;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);

  if (n > 0) {
    // Got data - call user's message callback
    // Use shared_from_this() to extend lifetime during callback
    messageCallback_(shared_from_this(), &inputBuffer_);
  } else if (n == 0) {
    // EOF
    handleClose();
  } else {
    handleError();
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();

  if (channel_->isWriting()) {
    const char *data = outputBuffer_.peek();
    size_t len = outputBuffer_.readableBytes();
    ssize_t n = ::write(channel_->fd(), data, len);

    if (n > 0) {
      outputBuffer_.retrieve(n);

      if (outputBuffer_.readableBytes() == 0) {
        // Add data sent
        channel_->disableWriting();

        if (state_ == State::Disconnecting) {
          shutdownInLoop();
        }
      }
    } else {
    }
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  assert(state_ == State::Connected || state_ == State::Disconnecting);

  state_ = State::Disconnected;
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());

  if (connectionCallback_) {
    connectionCallback_(guardThis);
  }

  if (closeCallback_) {
    closeCallback_(guardThis);
  }
}

void TcpConnection::handleError() {}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == State::Connecting);

  state_ = State::Connected;

  // Tie the channel to this connection
  channel_->tie(shared_from_this());

  channel_->enableReading();

  if (connectionCallback_) {
    connectionCallback_(shared_from_this());
  }
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();

  if (state_ == State::Connected) {
    channel_->disableAll();

    if (connectionCallback_) {
      connectionCallback_(shared_from_this());
    }
  }

  state_ = State::Disconnected;

  if (channel_->index() >= 0) {
    channel_->remove();
  }
}

void TcpConnection::shutdown() {
  if (state_ == State::Connected) {
    state_ = State::Disconnecting;
    loop_->runInLoop([this] { shutdownInLoop(); });
  }
}

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();

  if (!channel_->isWriting()) {
    socket_->shutdownWrite();
  }
}

void TcpConnection::forceClose() {
  if (state_ == State::Connected || state_ == State::Disconnecting) {
    state_ = State::Disconnecting;
    loop_->runInLoop([this] { handleClose(); });
  }
}

} // namespace hayai