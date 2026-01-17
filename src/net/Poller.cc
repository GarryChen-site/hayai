
#include "hayai/net/Poller.h"
#include "hayai/net/Channel.h"
#include <cassert>
#include <unistd.h>

namespace hayai {
Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop), kqueueFd_(::kqueue()), events_(16) {
  if (kqueueFd_ < 0) {
    // Fatal error
    abort();
  }
}

Poller::~Poller() { ::close(kqueueFd_); }

void Poller::poll(std::chrono::milliseconds timeout,
                  ChannelList *activeChannels) {
  struct timespec ts;
  ts.tv_sec = timeout.count() / 1000;
  ts.tv_nsec = (timeout.count() % 1000) * 1000000;

  int numEvents = ::kevent(kqueueFd_, nullptr, 0, events_.data(),
                           static_cast<int>(events_.size()), &ts);

  if (numEvents > 0) {
    fillActiveChannels(numEvents, activeChannels);
    if (static_cast<size_t>(numEvents) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents < 0) {
    if (errno != EINTR) {
      // Log error
    }
  }
}

void Poller::fillActiveChannels(int numEvents, ChannelList *activeChannels) {
  for (int i = 0; i < numEvents; ++i) {
    Channel *channel = static_cast<Channel *>(events_[i].udata);
    int revents = 0;

    if (events_[i].filter == EVFILT_READ) {
      revents |= Channel::kReadEvent;
    }
    if (events_[i].filter == EVFILT_WRITE) {
      revents |= Channel::kWriteEvent;
    }
    if (events_[i].flags & EV_EOF) {
      revents |= Channel::kCloseEvent;
    }
    if (events_[i].flags & EV_ERROR) {
      revents |= Channel::kErrorEvent;
    }

    channel->setRevents(revents);
    activeChannels->push_back(channel);
  }
}

void Poller::updateChannel(Channel *channel) {
  int fd = channel->fd();
  channels_[fd] = channel;

  struct kevent changes[2];
  int n = 0;

  if (channel->isReading()) {
    EV_SET(&changes[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, channel);
  } else {
    EV_SET(&changes[n++], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
  }

  if (channel->isWriting()) {
    EV_SET(&changes[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, channel);
  } else {
    EV_SET(&changes[n++], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
  }

  ::kevent(kqueueFd_, changes, n, nullptr, 0, nullptr);
}

void Poller::removeChannel(Channel *channel) {
  int fd = channel->fd();
  assert(channels_.find(fd) != channels_.end());
  channels_.erase(fd);

  struct kevent changes[2];
  EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
  EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
  ::kevent(kqueueFd_, changes, 2, nullptr, 0, nullptr);
}

} // namespace hayai