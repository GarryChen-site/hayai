#include "hayai/coro/AsyncConnection.h"
#include "hayai/net/EventLoop.h"

#include <iostream>

namespace hayai::coro {

AsyncConnection::AsyncConnection(TcpConnectionPtr conn)
    : conn_(std::move(conn)), loop_(conn_ ? conn_->getLoop() : nullptr) {
  if (conn_) {
    conn_->setMessageCallback([this](const TcpConnectionPtr &conn,
                                     Buffer *buf) { onMessage(conn, buf); });
    conn_->setWriteCompleteCallback(
        [this](const TcpConnectionPtr &conn) { onWriteComplete(conn); });
  }
}

AsyncConnection::~AsyncConnection() = default;

AsyncConnection::AsyncConnection(AsyncConnection &&other) noexcept
    : conn_(std::move(other.conn_)), loop_(other.loop_),
      recvCoroutine_(std::move(other.recvCoroutine_)),
      receivedData_(std::move(other.receivedData_)),
      sendCoroutine_(std::move(other.sendCoroutine_)) {
  other.loop_ = nullptr;
}

AsyncConnection &AsyncConnection::operator=(AsyncConnection &&other) noexcept {
  if (this != &other) {
    conn_ = std::move(other.conn_);
    loop_ = other.loop_;
    recvCoroutine_ = std::move(other.recvCoroutine_);
    receivedData_ = std::move(other.receivedData_);
    sendCoroutine_ = std::move(other.sendCoroutine_);
    other.loop_ = nullptr;
  }
  return *this;
}

void AsyncConnection::onMessage(const TcpConnectionPtr &conn, Buffer *buf) {
  std::lock_guard<std::mutex> lock(recvMutex_);

  receivedData_.append(buf->peek(), buf->readableBytes());
  buf->retrieveAll();

  if (recvCoroutine_) {
    auto h = *recvCoroutine_;
    recvCoroutine_.reset();

    loop_->queueInLoop([h]() mutable { h.resume(); });
  }
}

void AsyncConnection::onWriteComplete(const TcpConnectionPtr &conn) {
  std::lock_guard<std::mutex> lock(sendMutex_);

  if (sendCoroutine_) {
    auto h = *sendCoroutine_;
    sendCoroutine_.reset();

    loop_->queueInLoop([h]() mutable { h.resume(); });
  }
}

AsyncConnection::RecvAwaiter::RecvAwaiter(AsyncConnection &self)
    : self_(self) {}

bool AsyncConnection::RecvAwaiter::await_ready() const noexcept {

  std::lock_guard<std::mutex> lock(self_.recvMutex_);
  return self_.receivedData_.readableBytes() > 0;
}

void AsyncConnection::RecvAwaiter::await_suspend(std::coroutine_handle<> h) {
  std::lock_guard<std::mutex> lock(self_.recvMutex_);

  waitingCoroutine_ = h;

  if (self_.receivedData_.readableBytes() > 0) {
    self_.loop_->queueInLoop([h]() mutable { h.resume(); });
    return;
  }

  self_.recvCoroutine_ = h;
}

Buffer AsyncConnection::RecvAwaiter::await_resume() {
  std::lock_guard<std::mutex> lock(self_.recvMutex_);

  Buffer result = std::move(self_.receivedData_);
  self_.receivedData_ = Buffer{};
  return result;
}

AsyncConnection::SendAwaiter::SendAwaiter(AsyncConnection &self,
                                          std::string data)
    : self_(self), data_(std::move(data)) {}

bool AsyncConnection::SendAwaiter::await_ready() const noexcept {
  // Always suspend for send (need to wait for write complete)
  return false;
}

void AsyncConnection::SendAwaiter::await_suspend(std::coroutine_handle<> h) {
  std::lock_guard<std::mutex> lock(self_.sendMutex_);

  waiting_coroutine_ = h;
  self_.sendCoroutine_ = h;

  self_.conn_->send(data_);
}

void AsyncConnection::SendAwaiter::await_resume() {}

} // namespace hayai::coro