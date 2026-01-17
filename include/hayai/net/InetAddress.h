#pragma once

#include <netinet/in.h>
#include <string>
#include <string_view>

namespace hayai {

class InetAddress {
public:
  explicit InetAddress(uint16_t port = 0, bool loopback = false);
  InetAddress(std::string_view ip, uint16_t port);
  explicit InetAddress(const sockaddr_in &addr) : addr_(addr) {}

  [[nodiscard]] std::string toIp() const;
  [[nodiscard]] std::string toIpPort() const;
  [[nodiscard]] uint16_t port() const;

  [[nodiscard]] const sockaddr *getSockAddr() const {
    return reinterpret_cast<const sockaddr *>(&addr_);
  }

  void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

  // Comparison operators
  bool operator==(const InetAddress &rhs) const {
    return addr_.sin_family == rhs.addr_.sin_family &&
           addr_.sin_port == rhs.addr_.sin_port &&
           addr_.sin_addr.s_addr == rhs.addr_.sin_addr.s_addr;
  }

  bool operator!=(const InetAddress &rhs) const { return !(*this == rhs); }

private:
  sockaddr_in addr_;
};

} // namespace hayai