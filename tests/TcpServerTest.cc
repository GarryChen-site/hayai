#include "hayai/net/TcpServer.h"
#include "hayai/net/EventLoop.h"
#include "hayai/net/InetAddress.h"
#include <gtest/gtest.h>

namespace hayai {
namespace test {

class TcpServerTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TcpServerTest, Construction) {
  // Test basic construction
  EventLoop loop;
  InetAddress addr(9999);
  TcpServer server(&loop, addr, "TestServer");

  EXPECT_EQ(server.name(), "TestServer");
  EXPECT_EQ(server.getLoop(), &loop);
  EXPECT_FALSE(server.started());
}

TEST_F(TcpServerTest, SetIoLoopNum) {
  // Test setting I/O thread count
  EventLoop loop;
  InetAddress addr(9999);
  TcpServer server(&loop, addr, "TestServer");

  // Set I/O threads before start
  server.setIoLoopNum(4);

  EXPECT_FALSE(server.started());
}

TEST_F(TcpServerTest, CallbackSetters) {
  // Test that callbacks can be set
  EventLoop loop;
  InetAddress addr(9999);
  TcpServer server(&loop, addr, "TestServer");

  bool connectionCalled = false;
  bool messageCalled = false;
  bool writeCompleteCalled = false;

  server.setConnectionCallback(
      [&](const TcpConnectionPtr &) { connectionCalled = true; });

  server.setMessageCallback(
      [&](const TcpConnectionPtr &, Buffer *) { messageCalled = true; });

  server.setWriteCompleteCallback(
      [&](const TcpConnectionPtr &) { writeCompleteCalled = true; });

  // Just verify no crash - callbacks won't be invoked without actual
  // connections
  SUCCEED();
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
