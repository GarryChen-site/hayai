#include "hayai/coro/Awaiter.h"
#include "hayai/coro/Task.h"
#include <iostream>
#include <stdexcept>

using namespace hayai::coro;

Task<int> computeValue() {
  std::cout << "Computing..." << std::endl;
  co_return 42;
}

Task<int> multiplyByTwo(int x) { co_return x * 2; }

Task<int> computeAndDouble() {
  int value = co_await computeValue();
  std::cout << "Got value: " << value << std::endl;

  int doubled = co_await multiplyByTwo(value);
  co_return doubled;
}

Task<std::string> mightFail(bool shouldFail) {
  if (shouldFail) {
    throw std::runtime_error("Operation failed");
  }
  co_return "Success!";
}

Task<void> printMessage() {
  std::cout << "Hello from coroutine!" << std::endl;
  co_return;
}

int main() {
  std::cout << "=== Hayai Coroutine Examples ===" << std::endl << std::endl;

  // Example 1: Simple return
  std::cout << "Example 1: Simple return value" << std::endl;
  {
    auto task = computeValue();
    int result = task.get();
    std::cout << "Result: " << result << std::endl << std::endl;
  }

  // Example 2: Nested coroutines with co_await
  std::cout << "Example 2: Nested coroutines" << std::endl;
  {
    auto task = computeAndDouble();
    int result = task.get();
    std::cout << "Final result: " << result << std::endl << std::endl;
  }

  // Example 3: Exception handling
  std::cout << "Example 3: Exception handling" << std::endl;
  {
    try {
      auto task = mightFail(true);
      task.get();
    } catch (const std::exception &e) {
      std::cout << "Caught exception: " << e.what() << std::endl;
    }

    auto successTask = mightFail(false);
    std::cout << "Success case: " << successTask.get() << std::endl
              << std::endl;
  }

  // Example 4: Void return
  std::cout << "Example 4: Void coroutine" << std::endl;
  {
    auto task = printMessage();
    task.get();
    std::cout << std::endl;
  }

  std::cout << "=== All examples completed! ===" << std::endl;

  return 0;
}
