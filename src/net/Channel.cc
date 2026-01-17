#include "hayai/net/Channel.h"

namespace hayai {
Channel::Channel(EventLoop *loop, int fd) : loop_(loop), fd_(fd) {}

void Channel::tie(const std::shared_ptr<void> &obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::update() {}

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
  // 1. Error handling
  if ((revents_ & kErrorEvent) && errorCallback_) {
    errorCallback_();
  }
  // 2. Peer closed connection
  if ((revents_ & kCloseEvent) && !(revents_ & kReadEvent) && closeCallback_) {
    closeCallback_();
  }
  // 3. Read events
  if ((revents_ & kReadEvent) && readCallback_) {
    readCallback_();
  }
  // 4. Write events
  if ((revents_ & kWriteEvent) && writeCallback_) {
    writeCallback_();
  }
}
} // namespace hayai