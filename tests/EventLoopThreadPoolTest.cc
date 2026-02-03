#include "hayai/net/EventLoopThreadPool.h"
#include "hayai/net/EventLoop.h"
#include <gtest/gtest.h>
#include <set>

namespace hayai {
namespace test {

class EventLoopThreadPoolTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(EventLoopThreadPoolTest, Construction) {
  // Test basic construction
  EventLoop loop;
  EventLoopThreadPool pool(&loop, 0);

  EXPECT_FALSE(pool.started());
  EXPECT_EQ(pool.size(), 0);
}

TEST_F(EventLoopThreadPoolTest, SingleThreadedModeConstruction) {
  // Test construction with numThreads = 0 (single-threaded mode)
  EventLoop loop;
  EventLoopThreadPool pool(&loop, 0);

  // Start in loop thread
  loop.runInLoop([&]() {
    pool.start();

    EXPECT_TRUE(pool.started());
    EXPECT_EQ(pool.size(), 0); // No worker threads in single-threaded mode

    // Should return base loop
    EXPECT_EQ(pool.getNextLoop(), &loop);
    EXPECT_EQ(pool.getLoop(0), &loop);
    EXPECT_EQ(pool.getLoop(1), &loop);

    loop.quit();
  });

  loop.loop();
}

TEST_F(EventLoopThreadPoolTest, ThreadPoolSize) {
  // Test that pool creates correct number of threads
  EventLoop loop;
  const size_t numThreads = 4;
  EventLoopThreadPool pool(&loop, numThreads);

  EXPECT_FALSE(pool.started());

  loop.runInLoop([&]() {
    pool.start();

    EXPECT_TRUE(pool.started());
    EXPECT_EQ(pool.size(), numThreads);

    // Quit immediately to avoid hanging
    loop.quit();
  });

  loop.loop();

  // Destructor will clean up threads
}

TEST_F(EventLoopThreadPoolTest, GetLoopByIndex) {
  // Test getLoop() with valid and invalid indices
  EventLoop loop;
  const size_t numThreads = 3;
  EventLoopThreadPool pool(&loop, numThreads);

  loop.runInLoop([&]() {
    pool.start();

    // Valid indices
    for (size_t i = 0; i < numThreads; ++i) {
      EventLoop *threadLoop = pool.getLoop(i);
      EXPECT_NE(threadLoop, nullptr);
      EXPECT_NE(threadLoop, &loop); // Should be different from base loop
    }

    // Invalid index should return base loop (for now)
    // In Trantor it returns nullptr, but our implementation returns
    // baseLoop_
    EXPECT_EQ(pool.getLoop(numThreads), nullptr);

    loop.quit();
  });

  loop.loop();
}

TEST_F(EventLoopThreadPoolTest, UniqueEventLoops) {
  // Test that each thread gets a unique EventLoop
  EventLoop loop;
  const size_t numThreads = 4;
  EventLoopThreadPool pool(&loop, numThreads);

  loop.runInLoop([&]() {
    pool.start();

    std::set<EventLoop *> uniqueLoops;
    for (size_t i = 0; i < numThreads; ++i) {
      EventLoop *threadLoop = pool.getLoop(i);
      uniqueLoops.insert(threadLoop);
    }

    // Should have exactly numThreads unique loops
    EXPECT_EQ(uniqueLoops.size(), numThreads);

    // None should be the base loop
    EXPECT_EQ(uniqueLoops.count(&loop), 0);

    loop.quit();
  });

  loop.loop();
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
