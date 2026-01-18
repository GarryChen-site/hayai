#include "hayai/net/Acceptor.h"
#include "hayai/net/EventLoop.h"
#include <cassert>
#include <unistd.h>

namespace hayai {
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr,
                   bool reuseport)
    : loop_(loop), acceptSocket_(Socket::createTcpSocket()),
      acceptChannel_(loop, acceptSocket_.fd()), listenAddr_(listenAddr) {

  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);
  acceptSocket_.bind(listenAddr_);

  acceptChannel_.setReadCallback([this] { handleRead(); });
}

Acceptor::~Acceptor() {
  acceptChannel_.disableAll();
  acceptChannel_.remove();
}

void Acceptor::listen() {
  loop_->assertInLoopThread();
  listening_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();
  InetAddress peerAddr;
  int connfd = acceptSocket_.accept(&peerAddr);

  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      ::close(connfd);
    }
  } else {
    // keep simple
  }
}
} // namespace hayai