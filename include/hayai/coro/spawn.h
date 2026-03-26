#pragma once

/**
 * @brief spawn() - proper fire-and-forget coroutine launcher
 *
 * Wraps a Task<void> in a SpawnTask whose final_suspend destroys
 * the coroutine frame via EventLoop::cleanupSpawnedTask().
 *
 * Usage:
 *   // Instead of:
 *   handleClient(conn).detach();   // leaks frame if it suspends
 *
 *   // Use:
 *   spawn(&loop, handleClient(conn));  // proper ownership + cleanup
 */

#include "hayai/coro/Task.h"
#include "hayai/net/EventLoop.h"
#include <coroutine>

namespace hayai::coro {

/**
 * @brief SpawnTask - internal coroutine type used by spawn().
 *
 * A lightweight coroutine wrapper that:
 * - Always suspends initially (so the caller can register its handle
 *   in EventLoop::spawnedTasks_ before the first resume)
 * - At final_suspend, queues cleanupSpawnedTask() on the EventLoop
 *   so the frame is destroyed safely from the loop thread
 */
class SpawnTask {
  public:
    struct promise_type {
        EventLoop* loop_ = nullptr;

        SpawnTask get_return_object() {
            return SpawnTask{
                std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        // Suspend before first resume so caller can register the handle
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct FinalAwaiter {
            bool await_ready() noexcept { return false; }

            // At completion, queue self-destruction on the loop thread.
            void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                EventLoop* loop = h.promise().loop_;
                if (loop) {
                    // queueInLoop is always safe to call from any thread
                    loop->queueInLoop(
                        [loop, h]() mutable { loop->cleanupSpawnedTask(h); });
                }
            }

            void await_resume() noexcept {}
        };

        FinalAwaiter final_suspend() noexcept { return {}; }

        void return_void() noexcept {}

        void unhandled_exception() noexcept { std::terminate(); }
    };

    explicit SpawnTask(std::coroutine_handle<promise_type> h) : handle_(h) {}

    // Non-copyable, non-movable - the loop owns this handle
    SpawnTask(const SpawnTask&) = delete;
    SpawnTask& operator=(const SpawnTask&) = delete;
    SpawnTask(SpawnTask&&) = delete;
    SpawnTask& operator=(SpawnTask&&) = delete;

    std::coroutine_handle<> handle() const { return handle_; }

    ~SpawnTask() {}

  private:
    std::coroutine_handle<promise_type> handle_;
};

/**
 * @brief Spawn a fire-and-forget coroutine on an EventLoop.
 *
 * The coroutine is started on the EventLoop thread and its frame is
 * automatically cleaned up when it completes.
 *
 * Thread-safe. Can be called from any thread.
 *
 * @param loop  The EventLoop to run the coroutine on
 * @param task  Task<void> to run (ownership transferred)
 */

inline void spawn(EventLoop* loop, Task<void> task) {
    auto wrapper = [](EventLoop* l, Task<void> t) -> SpawnTask {
        co_await std::move(t);
    };

    SpawnTask spawnTask = wrapper(loop, std::move(task));

    auto typedHandle =
        std::coroutine_handle<SpawnTask::promise_type>::from_address(
            spawnTask.handle().address());
    typedHandle.promise().loop_ = loop;

    loop->spawn(spawnTask.handle());
}

} // namespace hayai::coro