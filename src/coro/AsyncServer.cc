#include "hayai/coro/AsyncServer.h"

namespace hayai::coro {

AsyncServer::AsyncServer(EventLoop *loop, const InetAddress &addr,
                         const std::string &name)
    : server_(loop, addr, name) {
  server_.setConnectionCallback([this](const TcpConnectionPtr &conn) {
    if (conn->connected()) {
      onConnection(conn);
    }
  });
}

AsyncServer::~AsyncServer() = default;

void AsyncServer::onConnection(const TcpConnectionPtr &conn) {

  AsyncConnection asyncConn(conn);

  std::lock_guard<std::mutex> lock(acceptMutex_);

  pendingConnections_.push_back(std::move(asyncConn));

  if (acceptCoroutine_) {
    auto h = *acceptCoroutine_;
    acceptCoroutine_.reset();

    server_.getLoop()->queueInLoop([h]() mutable { h.resume(); });
  }
}

AsyncServer::AcceptAwaiter::AcceptAwaiter(AsyncServer &self) : self_(self) {}

bool AsyncServer::AcceptAwaiter::await_ready() {
  std::lock_guard<std::mutex> lock(self_.acceptMutex_);
  return !self_.pendingConnections_.empty();
}

void AsyncServer::AcceptAwaiter::await_suspend(std::coroutine_handle<> h) {
  std::lock_guard<std::mutex> lock(self_.acceptMutex_);

  if (!self_.pendingConnections_.empty()) {
    self_.server_.getLoop()->queueInLoop([h]() mutable { h.resume(); });
    return;
  }

  self_.acceptCoroutine_ = h;
}

AsyncConnection AsyncServer::AcceptAwaiter::await_resume() {
  std::lock_guard<std::mutex> lock(self_.acceptMutex_);

  if (self_.pendingConnections_.empty()) {
    throw std::runtime_error("No pending connection (shouldn't happen)");
  }

  AsyncConnection conn = std::move(self_.pendingConnections_.front());
  self_.pendingConnections_.pop_front();
  return conn;
}
} // namespace hayai::coro