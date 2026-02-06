#pragma once

#include "hayai/utils/NonCopyable.h"
#include <system_error>

namespace hayai {
class InetAddress;

class Socket : NonCopyable {
public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}
  ~Socket();

  Socket(Socket &&other) noexcept : sockfd_(other.sockfd_) {
    other.sockfd_ = -1;
  }
  Socket &operator=(Socket &&other) noexcept {
    if (this != &other) {
      close();
      sockfd_ = other.sockfd_;
      other.sockfd_ = -1;
    }
    return *this;
  }

  [[nodiscard]] int fd() const { return sockfd_; }

  void setNonBlocking();
  void setReuseAddr(bool on = true);
  void setReusePort(bool on = true);
  void setTcpNoDelay(bool on = true);
  void setKeepAlive(bool on = true);

  void bind(const InetAddress &addr);
  void listen();

  int accept(InetAddress *peerAddr);

  void shutdownWrite();

  [[nodiscard]] static Socket createTcpSocket();
  [[nodiscard]] static InetAddress getLocalAddr(int sockfd);

private:
  void close();
  int sockfd_;
};
} // namespace hayai