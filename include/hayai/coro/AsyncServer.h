#pragma once

#include "hayai/coro/AsyncConnection.h"
#include "hayai/net/EventLoop.h"
#include "hayai/net/InetAddress.h"
#include "hayai/net/TcpConnection.h"
#include "hayai/net/TcpServer.h"
#include <coroutine>
#include <deque>
#include <mutex>
#include <optional>

namespace hayai::coro {

/**
 * @brief Async wrapper around TcpServer providing awaitable accept()
 *
 * Bridges callback-based TcpServer with coroutine-based AsyncServer:
 * - accept() suspends coroutine until new connection arrives
 * - Automatically handles EventLoop thread dispathcing
 * - Queues connections if no coroutine waiting
 */

class AsyncServer {
  public:
    AsyncServer(EventLoop* loop, const InetAddress& addr,
                const std::string& name);

    ~AsyncServer();

    // Non-copyable, non-movable (owns TcpServer)
    AsyncServer(const AsyncServer&) = delete;
    AsyncServer& operator=(const AsyncServer&) = delete;
    AsyncServer(AsyncServer&&) = delete;
    AsyncServer& operator=(AsyncServer&&) = delete;

    void start() { server_.start(); }

    void setIoLoopNum(size_t n) { server_.setIoLoopNum(n); }

    /**
     * Suspends coroutine until a new connection arrives, then returns
     * AsyncConnection
     */
    class AcceptAwaiter {
      public:
        explicit AcceptAwaiter(AsyncServer& self);

        bool await_ready();
        void await_suspend(std::coroutine_handle<> h);
        AsyncConnection await_resume();

      private:
        AsyncServer& self_;
    };

    AcceptAwaiter accept() { return AcceptAwaiter{*this}; }

    [[nodiscard]] const std::string& name() const { return server_.name(); }

    [[nodiscard]] EventLoop* getLoop() const { return server_.getLoop(); }

  private:
    friend class AcceptAwaiter;

    void onConnection(const TcpConnectionPtr& conn);

    TcpServer server_;

    std::mutex acceptMutex_;
    std::deque<AsyncConnection> pendingConnections_;
    std::optional<std::coroutine_handle<>> acceptCoroutine_;
};
} // namespace hayai::coro
