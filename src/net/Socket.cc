

#include "hayai/net/Socket.h"
#include "hayai/net/InetAddress.h"
#include <cstring>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
namespace hayai {

Socket::~Socket() { close(); }

void Socket::close() {
  if (sockfd_ >= 0) {
    ::close(sockfd_);
    sockfd_ = -1;
  }
}

Socket Socket::createTcpSocket() {
  int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    throw std::system_error(errno, std::system_category(),
                            "socket creation failed");
  }
  return Socket(sockfd);
}

void Socket::setNonBlocking() {
  int flags = ::fcntl(sockfd_, F_GETFL, 0);
  if (flags < 0) {
    throw std::system_error(errno, std::system_category(), "fcntl F_GETFL");
  }
  flags |= O_NONBLOCK;
  if (::fcntl(sockfd_, F_SETFL, flags) < 0) {
    throw std::system_error(errno, std::system_category(), "fcntl F_SETFL");
  }
}

void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) <
      0) {
    throw std::system_error(errno, std::system_category(),
                            "setsockopt SO_REUSEADDR");
  }
}

void Socket::setReusePort(bool on) {
  int optval = on ? 1 : 0;
  if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) <
      0) {
    throw std::system_error(errno, std::system_category(),
                            "setsockopt SO_REUSEPORT");
  }
}

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  if (::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) <
      0) {
    throw std::system_error(errno, std::system_category(),
                            "setsockopt TCP_NODELAY");
  }
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  if (::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) <
      0) {
    throw std::system_error(errno, std::system_category(),
                            "setsockopt SO_KEEPALIVE");
  }
}

void Socket::bind(const InetAddress &addr) {
  if (::bind(sockfd_, addr.getSockAddr(), sizeof(sockaddr_in)) < 0) {
    throw std::system_error(errno, std::system_category(), "bind");
  }
}

void Socket::listen() {
  if (::listen(sockfd_, SOMAXCONN) < 0) {
    throw std::system_error(errno, std::system_category(), "listen");
  }
}

int Socket::accept(InetAddress *peerAddr) {
  sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  std::memset(&addr, 0, sizeof(addr));

  int connfd = ::accept(sockfd_, reinterpret_cast<sockaddr *>(&addr), &addrlen);

  if (connfd >= 0) {
    peerAddr->setSockAddr(addr);
    // Set non-blocking manually since macOS doesn't have accept4
    int flags = ::fcntl(connfd, F_GETFL, 0);
    ::fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
  }

  return connfd;
}

void Socket::shutdownWrite() {
  if (::shutdown(sockfd_, SHUT_WR) < 0) {
  }
}

InetAddress Socket::getLocalAddr(int sockfd) {
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  std::memset(&addr, 0, sizeof(addr));

  if (::getsockname(sockfd, reinterpret_cast<sockaddr *>(&addr), &addrlen) <
      0) {
    throw std::system_error(errno, std::system_category(), "getsockname");
  }

  return InetAddress(addr);
}

} // namespace hayai