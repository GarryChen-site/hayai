#include "hayai/net/EventLoopThread.h"
#include "hayai/net/EventLoop.h"

namespace hayai {
EventLoopThread::EventLoopThread() = default;

EventLoopThread::~EventLoopThread() {
  if (loop_) {
    loop_->quit();
  }

  if (thread_.joinable()) {
    thread_.join();
  }
}

EventLoop *EventLoopThread::startLoop() {
  thread_ = std::thread([this] { threadFunc(); });

  // wait until EventLoop is created in the new thread
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return loop_ != nullptr; });
  }

  return loop_.get();
}

void EventLoopThread::threadFunc() {

  auto loop = std::make_unique<EventLoop>();

  // Notify waiting thread that EventLoop is ready
  {
    std::lock_guard<std::mutex> lock(mutex_);
    loop_ = std::move(loop);
    cond_.notify_one();
  }

  loop_->loop();
}

} // namespace hayai