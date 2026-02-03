#include "hayai/net/EventLoopThread.h"
#include "hayai/utils/NonCopyable.h"
#include <atomic>
#include <memory>
#include <vector>

namespace hayai {
class EventLoop;

class EventLoopThreadPool : NonCopyable {
public:
  EventLoopThreadPool(EventLoop *baseLoop, size_t numThreads = 0);
  ~EventLoopThreadPool();

  void start();

  EventLoop *getNextLoop();

  EventLoop *getLoop(size_t index);

  [[nodiscard]] size_t size() const { return loops_.size(); }

  [[nodiscard]] bool started() const { return started_; }

private:
  // Main / acceptor loop
  EventLoop *baseLoop_;
  size_t numThreads_;

  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;

  std::atomic<size_t> next_{0};
  bool started_{false};
};
} // namespace hayai