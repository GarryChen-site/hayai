#include "hayai/net/InetAddress.h"
#include <gtest/gtest.h>

TEST(InetAddressTest, Construction) {
  hayai::InetAddress addr1(8080);
  EXPECT_EQ(addr1.port(), 8080);

  hayai::InetAddress addr2("127.0.0.1", 9090);
  EXPECT_EQ(addr2.toIp(), "127.0.0.1");
  EXPECT_EQ(addr2.port(), 9090);
  EXPECT_EQ(addr2.toIpPort(), "127.0.0.1:9090");
}
TEST(InetAddressTest, Comparison) {
  hayai::InetAddress addr1("127.0.0.1", 8080);
  hayai::InetAddress addr2("127.0.0.1", 8080);
  hayai::InetAddress addr3("127.0.0.1", 9090);

  EXPECT_EQ(addr1, addr2);
  EXPECT_NE(addr1, addr3);
}