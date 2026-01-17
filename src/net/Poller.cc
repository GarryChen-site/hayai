
#include "hayai/net/Poller.h"
#include <system_error>
#include <unistd.h>

namespace hayai {
Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop), kqueueFd_(::kqueue()), events_(16) {
  if (kqueueFd_ < 0) {
    throw std::system_error(errno, std::system_category(),
                            "kqueue creation failed");
  }
}

Poller::~Poller() { ::close(kqueueFd_); }

void Poller::poll(std::chrono::milliseconds timeout,
                  ChannelList *activeChannels) {
  timespec ts;
  ts.tv_sec = timeout.count() / 1000;
  ts.tv_nsec = (timeout.count() % 1000) * 1000000;

  int numEvents = ::kevent(kqueueFd_, nullptr, 0, events_.data(),
                           static_cast<int>(events_.size()), &ts);

  if (numEvents < 0) {
    if (errno != EINTR) {
      throw std::system_error(errno, std::system_category(), "kevent failed");
    }
  } else if (numEvents > 0) {
    fillActiveChannels(numEvents, activeChannels);

    if (static_cast<size_t>(numEvents) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  }
}

void Poller::updateChannel(Channel *channel) {
  int fd = channel->fd();

  if (channels_.find(fd) == channels_.end()) {
    // New channel
    channels_[fd] = channel;
  }

  // Update kqueue filters;
  std::vector<struct kevent> changes;

  if (channel->isReading()) {
    struct kevent event;
    EV_SET(&event, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, channel);
    changes.push_back(event);
  } else {
    struct kevent event;
    EV_SET(&event, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    changes.push_back(event);
  }

  if (channel->isWriting()) {
    struct kevent event;
    EV_SET(&event, fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, channel);
    changes.push_back(event);
  } else {
    struct kevent event;
    EV_SET(&event, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
    changes.push_back(event);
  }

  if (!changes.empty()) {
    ::kevent(kqueueFd_, changes.data(), changes.size(), nullptr, 0, nullptr);
  }
}

void Poller::removeChannel(Channel *channel) {
  int fd = channel->fd();
  channels_.erase(fd);

  struct kevent changes[2];
  EV_SET(&changes[0], fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
  EV_SET(&changes[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
  ::kevent(kqueueFd_, changes, 2, nullptr, 0, nullptr);
}

void Poller::fillActiveChannels(int numEvents, ChannelList *activeChannels) {
  for (int i = 0; i < numEvents; ++i) {
    Channel *channel = static_cast<Channel *>(events_[i].udata);

    int events = 0;
    if (events_[i].filter == EVFILT_READ) {
      events |= Channel::kReadEvent;
    }

    if (events_[i].filter == EVFILT_WRITE) {
      events |= Channel::kWriteEvent;
    }

    if (events_[i].flags & EV_EOF) {
      events |= Channel::kCloseEvent;
    }

    if (events_[i].flags & EV_ERROR) {
      events |= Channel::kErrorEvent;
    }

    channel->setRevents(events);
    activeChannels->push_back(channel);
  }
}

} // namespace hayai