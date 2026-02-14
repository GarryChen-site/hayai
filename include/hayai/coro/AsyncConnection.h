#pragma once
#include "hayai/coro/Task.h"
#include "hayai/net/EventLoop.h"
#include "hayai/net/TcpConnection.h"
#include "hayai/utils/Buffer.h"
#include <coroutine>
#include <memory>
#include <mutex>
#include <optional>

namespace hayai::coro {

class AsyncConnection {
public:
  explicit AsyncConnection(TcpConnectionPtr conn);
  ~AsyncConnection();

  AsyncConnection(AsyncConnection &&other) noexcept;
  AsyncConnection &operator=(AsyncConnection &&other) noexcept;
  AsyncConnection(const AsyncConnection &) = delete;
  AsyncConnection &operator=(const AsyncConnection &) = delete;

  /**
   * @brief Awaitable receive operation.
   *
   * Suspends coroutine until data arrives, then returns the data.
   * Returns empty Buffer if connection is closed.
   */
  class RecvAwaiter {
  public:
    explicit RecvAwaiter(AsyncConnection &self);

    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h);
    Buffer await_resume();

  private:
    AsyncConnection &self_;
    std::coroutine_handle<> waitingCoroutine_;
  };

  RecvAwaiter recv() { return RecvAwaiter(*this); }

  /**
   * @brief Awaitable send operation.
   *
   * Suspends coroutine until data is fully written.
   */
  class SendAwaiter {
  public:
    SendAwaiter(AsyncConnection &self, std::string data);

    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> h);
    void await_resume();

  private:
    AsyncConnection &self_;
    std::string data_;
    std::coroutine_handle<> waiting_coroutine_;
  };

  SendAwaiter send(std::string data) {
    return SendAwaiter{*this, std::move(data)};
  }

  // Non-awaitable utilities
  [[nodiscard]] InetAddress localAddr() const {
    return conn_ ? conn_->localAddress() : InetAddress{};
  }

  [[nodiscard]] InetAddress peerAddr() const {
    return conn_ ? conn_->peerAddress() : InetAddress{};
  }

  [[nodiscard]] bool connected() const { return conn_ && conn_->connected(); }

  [[nodiscard]] const std::string &name() const {
    static const std::string empty;
    return conn_ ? conn_->name() : empty;
  }

private:
  friend class RecvAwaiter;
  friend class SendAwaiter;

  void onMessage(const TcpConnectionPtr &conn, Buffer *buf);
  void onWriteComplete(const TcpConnectionPtr &conn);

  TcpConnectionPtr conn_;
  EventLoop *loop_;

  std::mutex recvMutex_;
  std::optional<std::coroutine_handle<>> recvCoroutine_;
  Buffer receivedData_;

  std::mutex sendMutex_;
  std::optional<std::coroutine_handle<>> sendCoroutine_;
};
} // namespace hayai::coro