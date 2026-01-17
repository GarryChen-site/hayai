#include "hayai/net/EventLoop.h"
#include "hayai/net/Channel.h"
#include "hayai/net/Poller.h"
#include <cassert>
#include <fcntl.h>
#include <unistd.h>

namespace hayai {
thread_local EventLoop *t_loopInThisThread = nullptr;

EventLoop::EventLoop()
    : threadId_(std::this_thread::get_id()),
      poller_(std::make_unique<Poller>(this)) {
  if (t_loopInThisThread) {
    // One EventLoop per thread
    abort();
  } else {
    t_loopInThisThread = this;
  }

  // Create wakeup pipe
  if (::pipe(wakeupFd_) < 0) {
    abort();
  }

  // Set non-blocking
  int flags = ::fcntl(wakeupFd_[0], F_GETFL, 0);
  ::fcntl(wakeupFd_[0], F_SETFL, flags | O_NONBLOCK);

  wakeupChannel_ = std::make_unique<Channel>(this, wakeupFd_[0]);
  wakeupChannel_->setReadCallback([this] { handleWakeup(); });
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_[0]);
  ::close(wakeupFd_[1]);
  t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;

  while (!quit_) {
    activeChannels_.clear();
    poller_->poll(std::chrono::milliseconds(10000), &activeChannels_);

    eventHandling_ = true;
    for (Channel *channel : activeChannels_) {
      channel->handleEvent();
    }
    eventHandling_ = false;

    doPendingFunctors();
  }

  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  if (!isInLoopThread()) {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    std::scoped_lock lock(mutex_);
    pendingFunctors_.push_back(std::move(cb));
  }

  if (!isInLoopThread() || callingPendingFunctors_) {
    wakeup();
  }
}

void EventLoop::updateChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  poller_->removeChannel(channel);
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ::write(wakeupFd_[1], &one, sizeof(one));
}

void EventLoop::handleWakeup() {
  uint64_t one;
  ::read(wakeupFd_[0], &one, sizeof(one));
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::scoped_lock lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const auto &functor : functors) {
    functor();
  }
  callingPendingFunctors_ = false;
}

EventLoop *EventLoop::getEventLoopOfCurrentThread() {
  return t_loopInThisThread;
}

} // namespace hayai