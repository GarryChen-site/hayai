#pragma once

#include "hayai/coro/Task.h"
#include "hayai/net/EventLoop.h"
#include <coroutine>

namespace hayai::coro {

class SpawnTask {
public:
  struct promise_type {
    EventLoop *loop_ = nullptr;

    SpawnTask get_return_object() {
      return SpawnTask{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }

    struct FinalAwaiter {
      bool await_ready() noexcept { return false; }

      void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
        EventLoop *loop = h.promise().loop_;
        if (loop) {
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

  SpawnTask(const SpawnTask &) = delete;
  SpawnTask &operator=(const SpawnTask &) = delete;
  SpawnTask(SpawnTask &&) = delete;
  SpawnTask &operator=(SpawnTask &&) = delete;

  std::coroutine_handle<> handle() const { return handle_; }

  ~SpawnTask() {}

private:
  std::coroutine_handle<promise_type> handle_;
};

inline SpawnTask makeSpawnWrapper(EventLoop *loop, Task<void> task) {

  auto &promise = std::coroutine_handle<SpawnTask::promise_type>::from_address(
                      std::noop_coroutine().address())
                      .promise();
  (void)promise;

  co_await std::move(task);
}

inline void spawn(EventLoop *loop, Task<void> task) {
  auto wrapper = [](EventLoop *l, Task<void> t) -> SpawnTask {
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