#include "hayai/coro/Awaiter.h"
#include "hayai/coro/Task.h"
#include <gtest/gtest.h>
#include <string>

using namespace hayai::coro;

namespace hayai {
namespace test {

class CoroTaskTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test: Simple coroutine that returns a value
TEST_F(CoroTaskTest, SimpleReturnValue) {
  auto simpleTask = []() -> Task<int> { co_return 42; };

  Task<int> task = simpleTask();
  EXPECT_EQ(task.get(), 42);
}

// Test: Coroutine that returns void
TEST_F(CoroTaskTest, VoidReturn) {
  bool executed = false;

  auto voidTask = [&executed]() -> Task<void> {
    executed = true;
    co_return;
  };

  Task<void> task = voidTask();
  task.get();
  EXPECT_TRUE(executed);
}

// Test: Coroutine with string return
TEST_F(CoroTaskTest, StringReturn) {
  auto stringTask = []() -> Task<std::string> {
    co_return std::string("Hello, Coroutines!");
  };

  Task<std::string> task = stringTask();
  EXPECT_EQ(task.get(), "Hello, Coroutines!");
}

// Test: Exception handling
TEST_F(CoroTaskTest, ExceptionPropagation) {
  auto throwingTask = []() -> Task<int> {
    throw std::runtime_error("Test exception");
    co_return 0;
  };

  Task<int> task = throwingTask();
  EXPECT_THROW(task.get(), std::runtime_error);
}

// Test: Nested coroutines (co_await another task)
TEST_F(CoroTaskTest, NestedCoroutines) {
  auto innerTask = []() -> Task<int> { co_return 10; };

  auto outerTask = [&innerTask]() -> Task<int> {
    int value = co_await innerTask();
    co_return value * 2;
  };

  Task<int> task = outerTask();
  EXPECT_EQ(task.get(), 20);
}

// Test: Multiple co_await in sequence
TEST_F(CoroTaskTest, SequentialAwaits) {
  auto getValue = [](int x) -> Task<int> { co_return x; };

  auto sumTask = [&getValue]() -> Task<int> {
    int a = co_await getValue(5);
    int b = co_await getValue(7);
    int c = co_await getValue(3);
    co_return a + b + c;
  };

  Task<int> task = sumTask();
  EXPECT_EQ(task.get(), 15);
}

// Test: Move semantics
TEST_F(CoroTaskTest, MoveSemantics) {
  auto createTask = []() -> Task<int> { co_return 99; };

  Task<int> task1 = createTask();
  Task<int> task2 = std::move(task1);

  EXPECT_EQ(task2.get(), 99);
}

// Test: Awaiter utilities
TEST_F(CoroTaskTest, ImmediateValue) {
  auto immediateTask = []() -> Task<int> {
    int value = co_await makeReadyAwaiter(123);
    co_return value;
  };

  Task<int> task = immediateTask();
  EXPECT_EQ(task.get(), 123);
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
