
#include "hayai/net/Acceptor.h"
#include "hayai/net/EventLoop.h"
#include "hayai/net/InetAddress.h"
#include <arpa/inet.h>
#include <chrono>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace hayai {
namespace test {

class AcceptorTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(AcceptorTest, Construction) {
  EventLoop loop;
  InetAddress listenAddr(9999);
  Acceptor acceptor(&loop, listenAddr);

  EXPECT_FALSE(acceptor.listening());
  EXPECT_EQ(acceptor.address().port(), 9999);
}

TEST_F(AcceptorTest, ListenStatusChange) {
  EventLoop loop;
  InetAddress listenAddr(9998);
  Acceptor acceptor(&loop, listenAddr);

  // Start listening in the loop
  loop.runInLoop([&]() {
    acceptor.listen();
    EXPECT_TRUE(acceptor.listening());
    loop.quit();
  });

  loop.loop();
}

TEST_F(AcceptorTest, AcceptConnection) {
  EventLoop loop;
  InetAddress listenAddr(7776);
  Acceptor acceptor(&loop, listenAddr);

  int acceptedFd = -1;
  InetAddress acceptedAddr;

  acceptor.setNewConnectionCallback([&](int fd, const InetAddress &addr) {
    acceptedFd = fd;
    acceptedAddr = addr;
    loop.quit();
  });

  // Start server in separate thread
  std::thread serverThread([&]() {
    loop.runInLoop([&]() { acceptor.listen(); });
    loop.loop();
  });

  // Give server time to start
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Connect as client
  int clientFd = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(clientFd, 0);

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(7776);

  int ret =
      ::connect(clientFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
  EXPECT_EQ(ret, 0);

  serverThread.join();

  // Verify accepted connection
  EXPECT_GE(acceptedFd, 0);
  EXPECT_EQ(acceptedAddr.toIp(), "127.0.0.1");

  ::close(acceptedFd);
  ::close(clientFd);
}

TEST_F(AcceptorTest, MultipleConnections) {
  EventLoop loop;
  InetAddress listenAddr(7775);
  Acceptor acceptor(&loop, listenAddr);

  int connectionCount = 0;
  std::vector<int> acceptedFds;

  acceptor.setNewConnectionCallback([&](int fd, const InetAddress &) {
    acceptedFds.push_back(fd);
    connectionCount++;
    if (connectionCount >= 3) {
      loop.quit();
    }
  });

  std::thread serverThread([&]() {
    loop.runInLoop([&]() { acceptor.listen(); });
    loop.loop();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Create 3 client connections
  std::vector<int> clientFds;
  for (int i = 0; i < 3; ++i) {
    int clientFd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(clientFd, 0);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(7775);

    ::connect(clientFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    clientFds.push_back(clientFd);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  serverThread.join();

  EXPECT_EQ(connectionCount, 3);
  EXPECT_EQ(acceptedFds.size(), 3);

  // Cleanup
  for (int fd : acceptedFds)
    ::close(fd);
  for (int fd : clientFds)
    ::close(fd);
}

TEST_F(AcceptorTest, NoCallbackClosesConnection) {
  EventLoop loop;
  InetAddress listenAddr(7774);
  Acceptor acceptor(&loop, listenAddr);

  // Don't set a callback - connection should be closed automatically

  std::thread serverThread([&]() {
    loop.runInLoop([&]() { acceptor.listen(); });

    // Run for a short time then quit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    loop.quit();
    loop.loop();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Try to connect
  int clientFd = ::socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(clientFd, 0);

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(7774);

  ::connect(clientFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));

  // Connection should be closed by server
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  serverThread.join();
  ::close(clientFd);
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
