#include "hayai/net/TcpConnection.h"
#include "hayai/net/EventLoop.h"
#include "hayai/net/InetAddress.h"
#include <gtest/gtest.h>
#include <sys/socket.h>

namespace hayai {
namespace test {

class TcpConnectionTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(TcpConnectionTest, Construction) {
  // Test basic construction and initial state
  EventLoop loop;
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  InetAddress local(8080);
  InetAddress peer(9090);

  auto conn =
      std::make_shared<TcpConnection>(&loop, "conn1", fds[0], local, peer);

  // Initial state should be Connecting (not connected yet)
  EXPECT_FALSE(conn->connected());
  EXPECT_EQ(conn->localAddress().port(), 8080);
  EXPECT_EQ(conn->peerAddress().port(), 9090);

  // Clean up properly
  close(fds[1]);
  conn->connectDestroyed();
}

TEST_F(TcpConnectionTest, AddressGetters) {
  // Test address getters
  EventLoop loop;
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  InetAddress local(12345);
  InetAddress peer(54321);

  auto conn =
      std::make_shared<TcpConnection>(&loop, "conn2", fds[0], local, peer);

  EXPECT_EQ(conn->localAddress().port(), 12345);
  EXPECT_EQ(conn->peerAddress().port(), 54321);

  // Clean up
  close(fds[1]);
  conn->connectDestroyed();
}

TEST_F(TcpConnectionTest, CallbackSetters) {
  // Test that callbacks can be set without invoking them
  EventLoop loop;
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  InetAddress local(8080);
  InetAddress peer(9090);

  auto conn =
      std::make_shared<TcpConnection>(&loop, "conn3", fds[0], local, peer);

  // Set callbacks - just verify no crash
  conn->setConnectionCallback([](const TcpConnectionPtr &) {});
  conn->setMessageCallback([](const TcpConnectionPtr &, Buffer *) {});
  conn->setCloseCallback([](const TcpConnectionPtr &) {});

  // Clean up
  close(fds[1]);
  conn->connectDestroyed();
}

TEST_F(TcpConnectionTest, SendBeforeConnected) {
  // Test that send() before connected doesn't crash
  EventLoop loop;
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  InetAddress local(8080);
  InetAddress peer(9090);

  auto conn =
      std::make_shared<TcpConnection>(&loop, "conn4", fds[0], local, peer);

  // Should not crash (send is ignored when not connected)
  EXPECT_FALSE(conn->connected());
  conn->send("test message");

  // Clean up
  close(fds[1]);
  conn->connectDestroyed();
}

TEST_F(TcpConnectionTest, SharedPtrLifetime) {
  // Test that shared_ptr lifetime management works
  EventLoop loop;
  int fds[2];
  ASSERT_EQ(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);

  InetAddress local(8080);
  InetAddress peer(9090);

  std::weak_ptr<TcpConnection> weakConn;

  {
    auto conn =
        std::make_shared<TcpConnection>(&loop, "conn5", fds[0], local, peer);
    weakConn = conn;
    EXPECT_FALSE(weakConn.expired());

    conn->connectDestroyed();
  }

  // Connection should be destroyed
  EXPECT_TRUE(weakConn.expired());

  close(fds[1]);
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
