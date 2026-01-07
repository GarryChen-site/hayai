

#include "hayai/net/Socket.h"
#include "hayai/net/InetAddress.h"
#include <cstring>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
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
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

void Socket::bind(const InetAddress &addr) {
  if (::bind(sockfd_, addr.getSockAddr(), sizeof(sockaddr_in)) < 0) {
    throw std::system_error(errno, std::system_category(), "bind failed");
  }
}

void Socket::listen() {
  if (::listen(sockfd_, SOMAXCONN) < 0) {
    throw std::system_error(errno, std::system_category(), "listen failed");
  }
}

int Socket::accept(InetAddress *peerAddr) {
  sockaddr_in addr;
  socklen_t len = sizeof(addr);
  std::memset(&addr, 0, sizeof(addr));

  int connfd = ::accept(sockfd_, reinterpret_cast<sockaddr *>(&addr), &len);

  if (connfd >= 0) {
    peerAddr->setSockAddr(addr);
  }
  return connfd;
}

void Socket::shutdownWrite() {
  if (::shutdown(sockfd_, SHUT_WR) < 0) {
  }
}

} // namespace hayai