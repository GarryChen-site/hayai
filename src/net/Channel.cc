#include "hayai/net/Channel.h"
#include "hayai/net/EventLoop.h"

namespace hayai {
Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd) {}

Channel::~Channel() = default;

void Channel::tie(const std::shared_ptr<void> &obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::update() { loop_->updateChannel(this); }

void Channel::remove() { loop_->removeChannel(this); }

void Channel::handleEvent() {
  if (tied_) {
    if (auto guard = tie_.lock()) {
      handleEventWithGuard();
    }
  } else {
    handleEventWithGuard();
  }
}

void Channel::handleEventWithGuard() {
  if (revents_ & kErrorEvent) {
    if (errorCallback_)
      errorCallback_();
  }
  if (revents_ & kCloseEvent) {
    if (closeCallback_)
      closeCallback_();
  }
  if (revents_ & kReadEvent) {
    if (readCallback_)
      readCallback_();
  }
  if (revents_ & kWriteEvent) {
    if (writeCallback_)
      writeCallback_();
  }
}
} // namespace hayai