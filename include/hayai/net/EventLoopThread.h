#include "hayai/utils/NonCopyable.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace hayai {
class EventLoop;

/**
 * @brief EventLoopThread runs an EventLoop in a separate thread.
 */

class EventLoopThread : NonCopyable {
  public:
    EventLoopThread();
    ~EventLoopThread();

    /**
     * @brief Start the thread and return the EventLoop pointer.
     *
     * Blocks until the EventLoop is fully created in the new thread.
     * This ensures the returned EventLoop* is safe to use.
     *
     * @return EventLoop* pointer to the EventLoop in the new thread
     */
    EventLoop *startLoop();

  private:
    void threadFunc();

    std::unique_ptr<EventLoop> loop_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
};
} // namespace hayai