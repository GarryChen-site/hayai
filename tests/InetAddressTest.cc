#include "hayai/net/InetAddress.h"
#include <gtest/gtest.h>

namespace hayai {
namespace test {

class InetAddressTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(InetAddressTest, ConstructorWithPort) {
  InetAddress addr(8080);
  EXPECT_EQ(addr.port(), 8080);
  EXPECT_EQ(addr.toIp(), "0.0.0.0");
}

TEST_F(InetAddressTest, ConstructorWithPortAndLoopback) {
  InetAddress addr(9090, true);
  EXPECT_EQ(addr.port(), 9090);
  EXPECT_EQ(addr.toIp(), "127.0.0.1");
}

TEST_F(InetAddressTest, ConstructorWithIpAndPort) {
  InetAddress addr("192.168.1.100", 3000);
  EXPECT_EQ(addr.port(), 3000);
  EXPECT_EQ(addr.toIp(), "192.168.1.100");
}

TEST_F(InetAddressTest, ToIpPort) {
  InetAddress addr("10.0.0.1", 8080);
  EXPECT_EQ(addr.toIpPort(), "10.0.0.1:8080");
}

TEST_F(InetAddressTest, LocalhostAddress) {
  InetAddress localhost("127.0.0.1", 8000);
  EXPECT_EQ(localhost.toIp(), "127.0.0.1");
  EXPECT_EQ(localhost.port(), 8000);
}

TEST_F(InetAddressTest, ZeroPort) {
  InetAddress addr(0);
  EXPECT_EQ(addr.port(), 0);
}

TEST_F(InetAddressTest, SockAddrConversion) {
  InetAddress addr("172.16.0.1", 443);
  const sockaddr *sa = addr.getSockAddr();
  EXPECT_NE(sa, nullptr);

  // Verify it's an IPv4 address
  const sockaddr_in *sin = reinterpret_cast<const sockaddr_in *>(sa);
  EXPECT_EQ(sin->sin_family, AF_INET);
}

TEST_F(InetAddressTest, Equality) {
  InetAddress addr1("192.168.1.1", 8080);
  InetAddress addr2("192.168.1.1", 8080);
  InetAddress addr3("192.168.1.2", 8080);

  EXPECT_EQ(addr1, addr2);
  EXPECT_NE(addr1, addr3);
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
