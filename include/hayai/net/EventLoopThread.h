#include "hayai/utils/NonCopyable.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace hayai {
class EventLoop;

class EventLoopThread : NonCopyable {
public:
  EventLoopThread();
  ~EventLoopThread();

  EventLoop *startLoop();

private:
  void threadFunc();

  std::unique_ptr<EventLoop> loop_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
};
} // namespace hayai