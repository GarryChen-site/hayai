#include "hayai/coro/Task.h"
#include "hayai/coro/spawn.h"
#include "hayai/net/EventLoop.h"
#include <future>
#include <gtest/gtest.h>
using namespace hayai;
using namespace hayai::coro;

namespace {
class SpawnTest : public ::testing::Test {

protected:
  void SetUp() override {}

  void TearDown() override {}
};

struct LoopThread {
  std::promise<EventLoop *> loopPromise;
  std::thread thread;
  EventLoop *loop = nullptr;

  void start() {
    thread = std::thread([this] {
      EventLoop loopObj;
      loopPromise.set_value(&loopObj);
      loopObj.loop();
    });
    loop = loopPromise.get_future().get();
  }

  void stop() {
    if (loop) {
      loop->quit();
    }
    if (thread.joinable()) {
      thread.join();
    }
  }

  ~LoopThread() { stop(); }
};

TEST_F(SpawnTest, BasicSpawn) {
  std::atomic<int> counter{0};
  LoopThread lt;
  lt.start();

  auto task = [&]() -> Task<void> {
    counter.fetch_add(1);
    co_return;
  };

  spawn(lt.loop, task());
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  lt.stop();
  EXPECT_EQ(counter.load(), 1);
}

TEST_F(SpawnTest, MultipleSpawns) {
  std::atomic<int> counter{0};
  const int N = 5;
  LoopThread lt;
  lt.start();

  auto task = [&]() -> Task<void> {
    counter.fetch_add(1);
    co_return;
  };

  for (int i = 0; i < N; i++) {
    spawn(lt.loop, task());
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  lt.stop();

  EXPECT_EQ(counter.load(), N);
}

TEST_F(SpawnTest, SpawnFromOtherThread) {
  std::atomic<int> counter{0};
  LoopThread lt;
  lt.start();

  auto task = [&]() -> Task<void> {
    counter.fetch_add(1);
    co_return;
  };

  // This is always called from main/test thread (non-loop thread)
  spawn(lt.loop, task());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  lt.stop();

  EXPECT_EQ(counter.load(), 1);
}

TEST_F(SpawnTest, CleanShutdown) {
  std::atomic<int> completed{0};
  LoopThread lt;
  lt.start();

  auto task = [&]() -> Task<void> {
    completed.fetch_add(1);
    co_return;
  };

  for (int i = 0; i < 3; i++) {
    spawn(lt.loop, task());
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  lt.stop();

  EXPECT_EQ(completed.load(), 3);
}

} // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}