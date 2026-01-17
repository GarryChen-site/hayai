#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
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
};

} // namespace hayai