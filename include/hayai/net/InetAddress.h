#pragma once

#include <cstring>
#include <format>
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

  bool operator==(const InetAddress &other) const {
    return memcmp(&addr_, &other.addr_, sizeof(addr_)) == 0;
  }

  bool operator!=(const InetAddress &other) const { return !(*this == other); }

private:
  sockaddr_in addr_;
};

} // namespace hayai