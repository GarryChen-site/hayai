
#include <coroutine>
#include <utility>
namespace hayai::coro {

struct SuspendAlways {
  bool await_ready() const noexcept { return false; }

  void await_suspend(std::coroutine_handle<>) const noexcept {}

  void await_resume() const noexcept {}
};

struct SuspendNever {
  bool await_ready() const noexcept { return true; }

  void await_suspend(std::coroutine_handle<>) const noexcept {}

  void await_resume() const noexcept {}
};

template <typename T> struct AwaiterBase {
  T value_;

  explicit AwaiterBase(T value) : value_(std::move(value)) {}

  bool await_ready() const noexcept { return true; }

  void await_suspend(std::coroutine_handle<>) const noexcept {}

  T await_resume() { return std::move(value_); }
};

template <typename T> auto makeReadyAwaiter(T value) {
  return AwaiterBase<T>(std::move(value));
}
} // namespace hayai::coro