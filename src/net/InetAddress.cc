
#include "hayai/net/InetAddress.h"
#include <arpa/inet.h>
#include <cstring>
#include <sstream>

namespace hayai {

InetAddress::InetAddress(uint16_t port, bool loopback) {
  std::memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = htonl(loopback ? INADDR_LOOPBACK : INADDR_ANY);
  addr_.sin_port = htons(port);
}

InetAddress::InetAddress(std::string_view ip, uint16_t port) {
  std::memset(&addr_, 0, sizeof(addr_));
  addr_.sin_family = AF_INET;
  addr_.sin_port = htons(port);
  std::string ip_str(ip);
  ::inet_pton(AF_INET, ip_str.c_str(), &addr_.sin_addr);
}

std::string InetAddress::toIp() const {
  char buf[INET_ADDRSTRLEN];
  ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
  return buf;
}

std::string InetAddress::toIpPort() const {
  std::ostringstream oss;
  oss << toIp() << ":" << port();
  return oss.str();
}

uint16_t InetAddress::port() const { return ntohs(addr_.sin_port); }

} // namespace hayai
