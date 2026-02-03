#include "hayai/net/EventLoopThreadPool.h"
#include "hayai/net/EventLoop.h"
#include <cassert>
#include <memory>

namespace hayai {
EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, size_t numThreads)
    : baseLoop_(baseLoop), numThreads_(numThreads) {
  assert(baseLoop != nullptr);
}

EventLoopThreadPool::~EventLoopThreadPool() {
  // Threads are automatically joined in EventLoopThread destructor
}

void EventLoopThreadPool::start() {
  assert(!started_);
  baseLoop_->assertInLoopThread();

  started_ = true;

  if (numThreads_ == 0) {
    // single-threafed mode - no worker threads
    return;
  }

  // Create worker threads
  threads_.reserve(numThreads_);
  loops_.reserve(numThreads_);

  for (size_t i = 0; i < numThreads_; i++) {
    auto thread = std::make_unique<EventLoopThread>();
    EventLoop *loop = thread->startLoop();

    threads_.push_back(std::move(thread));
    loops_.push_back(loop);
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  baseLoop_->assertInLoopThread();
  assert(started_);

  if (loops_.empty()) {
    return baseLoop_;
  }

  size_t index = next_.fetch_add(1) % loops_.size();
  return loops_[index];
}

EventLoop *EventLoopThreadPool::getLoop(size_t index) {
  baseLoop_->assertInLoopThread();
  assert(started_);

  if (loops_.empty()) {
    return baseLoop_;
  }

  if (index >= loops_.size()) {
    return nullptr;
  }

  return loops_[index];
}
} // namespace hayai