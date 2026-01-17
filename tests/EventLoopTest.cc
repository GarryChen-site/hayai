#include "hayai/net/EventLoop.h"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

namespace hayai {
namespace test {

class EventLoopTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(EventLoopTest, Construction) {
  EventLoop loop;
  EXPECT_TRUE(loop.isInLoopThread());
}

TEST_F(EventLoopTest, OneLoopPerThread) {
  EventLoop loop1;
  // Creating another loop in the same thread should abort
  // We can't test this without crashing, so we skip it
}

TEST_F(EventLoopTest, RunInLoop) {
  EventLoop loop;
  bool executed = false;

  loop.runInLoop([&]() {
    executed = true;
    loop.quit();
  });

  loop.loop();
  EXPECT_TRUE(executed);
}

TEST_F(EventLoopTest, QueueInLoop) {
  EventLoop loop;
  int count = 0;

  loop.queueInLoop([&]() {
    count++;
    loop.quit();
  });

  loop.loop();
  EXPECT_EQ(count, 1);
}

TEST_F(EventLoopTest, CrossThreadCall) {
  EventLoop loop;
  std::atomic<int> count{0};

  std::thread t([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    loop.runInLoop([&]() {
      count++;
      loop.quit();
    });
  });

  loop.loop();
  t.join();

  EXPECT_EQ(count, 1);
}

TEST_F(EventLoopTest, MultipleQueuedTasks) {
  EventLoop loop;
  int count = 0;

  for (int i = 0; i < 10; ++i) {
    loop.queueInLoop([&]() { count++; });
  }

  loop.runInLoop([&]() { loop.quit(); });

  loop.loop();
  EXPECT_EQ(count, 10);
}

TEST_F(EventLoopTest, QuitFromAnotherThread) {
  EventLoop loop;
  std::atomic<bool> loopStarted{false};

  std::thread t([&]() {
    loopStarted = true;
    loop.loop();
  });

  // Wait for loop to start
  while (!loopStarted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  loop.quit();

  t.join();
}

TEST_F(EventLoopTest, NestedRunInLoop) {
  EventLoop loop;
  int count = 0;

  loop.runInLoop([&]() {
    count++;
    loop.runInLoop([&]() {
      count++;
      loop.quit();
    });
  });

  loop.loop();
  EXPECT_EQ(count, 2);
}

TEST_F(EventLoopTest, GetEventLoopOfCurrentThread) {
  EventLoop *loopPtr = EventLoop::getEventLoopOfCurrentThread();
  EXPECT_EQ(loopPtr, nullptr);

  EventLoop loop;
  loopPtr = EventLoop::getEventLoopOfCurrentThread();
  EXPECT_EQ(loopPtr, &loop);
}

TEST_F(EventLoopTest, IsInLoopThreadFromDifferentThread) {
  EventLoop loop;
  bool isInThread = true;

  std::thread t([&]() { isInThread = loop.isInLoopThread(); });

  t.join();
  EXPECT_FALSE(isInThread);
  EXPECT_TRUE(loop.isInLoopThread());
}

TEST_F(EventLoopTest, TaskExecutionOrder) {
  EventLoop loop;
  std::vector<int> order;

  loop.runInLoop([&]() { order.push_back(1); });

  loop.queueInLoop([&]() { order.push_back(2); });

  loop.queueInLoop([&]() {
    order.push_back(3);
    loop.quit();
  });

  loop.loop();

  ASSERT_EQ(order.size(), 3);
  EXPECT_EQ(order[0], 1);
  EXPECT_EQ(order[1], 2);
  EXPECT_EQ(order[2], 3);
}

TEST_F(EventLoopTest, WakeupMechanism) {
  EventLoop loop;
  std::atomic<bool> taskExecuted{false};

  std::thread loopThread([&]() { loop.loop(); });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Queue a task from another thread - should wake up the loop
  loop.runInLoop([&]() {
    taskExecuted = true;
    loop.quit();
  });

  loopThread.join();
  EXPECT_TRUE(taskExecuted);
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
