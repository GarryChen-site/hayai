#include "hayai/net/TcpServer.h"
#include "hayai/net/Acceptor.h"
#include "hayai/net/EventLoop.h"
#include "hayai/net/Socket.h"
#include <cassert>

namespace hayai {
TcpServer::TcpServer(EventLoop *loop, const InetAddress &addr, std::string name)
    : loop_(loop), name_(std::move(name)),
      acceptor_(std::make_unique<Acceptor>(loop, addr, true)),
      threadPool_(std::make_unique<EventLoopThreadPool>(loop, 0)) {
  assert(loop != nullptr);

  acceptor_->setNewConnectionCallback(
      [this](int sockfd, const InetAddress &peerAddr) {
        newConnection(sockfd, peerAddr);
      });
}

TcpServer::~TcpServer() {
  // Destructor may be called outside the loop thread
  // The server should have been stooped before destruction
  for (auto &item : connections_) {
    item.second->connectDestroyed();
  }
}

void TcpServer::setIoLoopNum(size_t num) {
  assert(!started_);
  threadPool_ = std::make_unique<EventLoopThreadPool>(loop_, num);
}

void TcpServer::start() {
  if (started_.exchange(true)) {
    return;
  }

  threadPool_->start();

  loop_->runInLoop([this]() { acceptor_->listen(); });
}

void TcpServer::stop() {
  started_ = false;

  loop_->runInLoop([this]() {
    acceptor_.reset();

    // Close all existing connections
    // Copy to vector to avoid iterator invalidation
    std::vector<TcpConnectionPtr> conns;
    conns.reserve(connections_.size());
    for (auto &item : connections_) {
      conns.push_back(item.second);
    }

    for (auto &conn : conns) {
      conn->shutdown();
    }
  });
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
  loop_->assertInLoopThread();

  // Get next I/O loop (round-robin accross thread pool)
  EventLoop *ioLoop = threadPool_->getNextLoop();

  // Create unique connection name
  std::string connName = std::format("{}#{}", name_, nextConnId_++);

  InetAddress localAddr(Socket::getLocalAddr(sockfd));

  // Create TcpConnection (shared_ptr for lifetime management)
  auto conn = std::make_shared<TcpConnection>(ioLoop, connName, sockfd,
                                              localAddr, peerAddr);

  // Store in map
  connections_[connName] = conn;

  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);

  conn->setCloseCallback(
      [this](const TcpConnectionPtr &c) { removeConnection(c); });

  // Establish connection in its I/O thread
  ioLoop->runInLoop([conn]() { conn->connectEstablished(); });
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn) {
  // This may be called from I/O thread, so queue to acceptor loop
  loop_->runInLoop([this, conn]() { removeConnectionInLoop(conn); });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn) {
  loop_->assertInLoopThread();

  size_t n = connections_.erase(conn->name());

  if (n == 0) {
    return;
  }

  // Destory connection in its I/O thread
  // Use queueInLoop (not runInLoop) because this connection may be
  // in its loop's current active channels, waiting to be processed.
  // Calling connectDestroyed() immediately would use a dangling pointer.
  EventLoop *ioLoop = conn->getLoop();
  ioLoop->queueInLoop([conn]() { conn->connectDestroyed(); });
}
} // namespace hayai