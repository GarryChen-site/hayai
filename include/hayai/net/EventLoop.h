#pragma once
#include <atomic>
#include <coroutine>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "hayai/utils/NonCopyable.h"

namespace hayai {
class Poller;
class Channel;

class EventLoop : NonCopyable {
public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  // The main loop: blocks in Poller until events occur
  void loop();
  void quit();

  void runInLoop(Functor cb);
  void queueInLoop(Functor cb);

  /**
   * @brief Spawn a fire-and-forget coroutine on this EventLoop.
   *
   * Unlike Task::detach(), spawn() properly owns the coroutine frame
   * for its entire lifetime, cleaning up automatically on completion.
   * Thread-safe - can be called from any thread.
   *
   * @param task A Task<void> to run. Ownership is transferred.
   */
  void spawn(std::coroutine_handle<> handle);

  /**
   * @brief Internal: called by SpawnTask::FinalAwaiter to destroy
   * the coroutine frame when it completes. Must be called on loop thread.
   * (Public so spawn.h's FinalAwaiter can reach it without friendship.)
   */
  void cleanupSpawnedTask(std::coroutine_handle<> handle);

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);

  [[nodiscard]] bool isInLoopThread() const {
    return threadId_ == std::this_thread::get_id();
  }

  void assertInLoopThread() const {
    if (!isInLoopThread()) {
      abort();
    }
  }

  static EventLoop *getEventLoopOfCurrentThread();

private:
  void wakeup();
  void handleWakeup(); // Read from wakeup pipe
  void doPendingFunctors();

  std::atomic<bool> looping_{false};
  std::atomic<bool> quit_{false};
  std::atomic<bool> eventHandling_{false};
  std::atomic<bool> callingPendingFunctors_{false};

  const std::thread::id threadId_;
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<Channel> wakeupChannel_;
  int wakeupFd_[2]; // pipe for wakeup

  std::vector<Channel *> activeChannels_;

  mutable std::mutex mutex_;
  std::vector<Functor> pendingFunctors_;

  std::set<std::coroutine_handle<>> spawnedTasks_;
};

} // namespace hayai