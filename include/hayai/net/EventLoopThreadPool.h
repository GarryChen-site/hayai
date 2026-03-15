#include "hayai/net/EventLoopThread.h"
#include "hayai/utils/NonCopyable.h"
#include <atomic>
#include <memory>
#include <vector>

namespace hayai {
class EventLoop;

/**
 * @brief EventLoopThreadPool manages a pool of EventLoop threads.

 * Thread Distribution:
 * Main Thread:           Worker Thread 1:      Worker Thread 2:
 *   EventLoop (accept)     EventLoop             EventLoop
 *     ↓                      ↓                     ↓
 *   Acceptor            Connections 1,3,5     Connections 2,4,6
 */
class EventLoopThreadPool : NonCopyable {
  public:
    EventLoopThreadPool(EventLoop *baseLoop, size_t numThreads = 0);
    ~EventLoopThreadPool();

    /**
     * @brief Start all I/O threads
     *
     * Must be called before getNextLoop().
     * Creates EventLoop in each thread.
     */
    void start();

    /**
     * @brief Get next EventLoop using round-robin
     *
     * Thread-safe. Uses atomic counter for lock-free distribution.
     *
     * @return EventLoop* for handling next connection
     */
    EventLoop *getNextLoop();

    EventLoop *getLoop(size_t index);

    [[nodiscard]] size_t size() const { return loops_.size(); }

    [[nodiscard]] bool started() const { return started_; }

  private:
    // Main / acceptor loop
    EventLoop *baseLoop_;
    size_t numThreads_;

    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    // Raw pointers for quick access
    std::vector<EventLoop *> loops_;

    std::atomic<size_t> next_{0};
    bool started_{false};
};
} // namespace hayai