#pragma once

#include "hayai/utils/NonCopyable.h"
#include <chrono>
#include <map>
#include <sys/event.h>
#include <vector>

namespace hayai {
class Channel;
class EventLoop;

class Poller : NonCopyable {
public:
  using ChannelList = std::vector<Channel *>;

  explicit Poller(EventLoop *loop);
  ~Poller();

  void poll(std::chrono::milliseconds timeout, ChannelList *activeChannels);

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);

private:
  void fillActiveChannels(int numEvents, ChannelList *activeChannels);

  EventLoop *ownerLoop_;
  int kqueueFd_;
  std::vector<struct kevent> events_; // For kevent() resutls
  std::map<int, Channel *> channels_; // fd -> Channel mapping
};
} // namespace hayai