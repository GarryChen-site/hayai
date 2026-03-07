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

// ── Helper: run EventLoop on a dedicated thread ────────────────────────────
//
// EventLoop must be constructed on the same thread that calls loop().
// We accomplish this by constructing it in a background thread and
// communicating the pointer back via a promise.

struct LoopThread {
  std::promise<EventLoop *> loopPromise;
  std::thread thread;
  EventLoop *loop = nullptr;

  void start() {
    thread = std::thread([this] {
      EventLoop loopObj;
      loopPromise.set_value(&loopObj);
      loopObj.loop(); // Blocks until quit()
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
    loop = nullptr;
  }

  ~LoopThread() { stop(); }
};

// ── Tests ──────────────────────────────────────────────────────────────────

/**
 * Basic: spawn a simple coroutine that completes synchronously.
 */
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

/**
 * Multiple spawns: each coroutine should run independently.
 */
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

/**
 * Cross-thread: spawn() called from non-loop thread (always true here).
 */
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

/**
 * Clean shutdown: loop exits cleanly after spawned tasks complete.
 */
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