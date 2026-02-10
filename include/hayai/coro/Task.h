#include <coroutine>
#include <exception>
#include <stdexcept>
#include <utility>

namespace hayai::coro {
template <typename T> class Task;

template <typename T = void> class Task {
public:
  struct promise_type {
    T value_;
    std::exception_ptr exception_;
    std::coroutine_handle<> continuation_;

    Task get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }

    struct FinalAwaiter {
      bool await_ready() noexcept { return false; }

      std::coroutine_handle<>
      await_suspend(std::coroutine_handle<promise_type> h) noexcept {
        auto &promise = h.promise();
        if (promise.continuation_) {
          return promise.continuation_;
        }
        return std::noop_coroutine();
      }

      void await_resume() noexcept {}
    };
    FinalAwaiter final_suspend() noexcept { return {}; }

    void return_void() noexcept {}

    void unhandled_exception() { exception_ = std::current_exception(); }
  };

  explicit Task(std::coroutine_handle<promise_type> h) : handle_(h) {}

  Task(Task &&other) noexcept : handle_(std::exchange(other.handle_, {})) {}

  Task &operator=(Task &&other) noexcept {
    if (this != &other) {
      if (handle_) {
        handle_.destroy();
      }
      handle_ = std::exchange(other.handle_, {});
    }
    return *this;
  }

  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;

  ~Task() {
    if (handle_) {
      handle_.destroy();
    }
  }

  bool await_ready() const noexcept { return false; }

  std::coroutine_handle<>
  await_suspend(std::coroutine_handle<> continuation) noexcept {
    handle_.promise().continuation_ = continuation;
    return handle_;
  }

  void await_resume() {
    if (handle_.promise().exception_) {
      std::rethrow_exception(handle_.promise().exception_);
    }
  }

  void get() {
    if (!handle_) {
      throw std::runtime_error("Task is empty");
    }

    if (!handle_.done()) {
      handle_.resume();
    }

    while (!handle_.done()) {
      handle_.resume();
    }

    await_resume();
  }

  std::coroutine_handle<promise_type> handle() const noexcept {
    return handle_;
  }

  void detach() {
    if (handle_) {
      handle_.resume();
      handle_ = nullptr;
    }
  }

private:
  std::coroutine_handle<promise_type> handle_;
};

} // namespace hayai::coro