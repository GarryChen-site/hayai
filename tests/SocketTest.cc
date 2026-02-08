#include "hayai/net/Socket.h"
#include "hayai/net/InetAddress.h"
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <thread>

namespace hayai {
namespace test {

class SocketTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(SocketTest, CreateTcpSocket) {
  auto sock = Socket::createTcpSocket();
  EXPECT_GE(sock.fd(), 0);
}

TEST_F(SocketTest, MoveConstructor) {
  auto sock1 = Socket::createTcpSocket();
  int fd1 = sock1.fd();

  Socket sock2(std::move(sock1));
  EXPECT_EQ(sock2.fd(), fd1);
  EXPECT_EQ(sock1.fd(), -1);
}

TEST_F(SocketTest, MoveAssignment) {
  auto sock1 = Socket::createTcpSocket();
  auto sock2 = Socket::createTcpSocket();
  int fd1 = sock1.fd();

  sock2 = std::move(sock1);
  EXPECT_EQ(sock2.fd(), fd1);
  EXPECT_EQ(sock1.fd(), -1);
}

TEST_F(SocketTest, SetNonBlocking) {
  auto sock = Socket::createTcpSocket();
  EXPECT_NO_THROW(sock.setNonBlocking());

  // Verify non-blocking by checking fcntl flags
  int flags = ::fcntl(sock.fd(), F_GETFL, 0);
  EXPECT_NE(flags & O_NONBLOCK, 0);
}

TEST_F(SocketTest, SetReuseAddr) {
  auto sock = Socket::createTcpSocket();
  EXPECT_NO_THROW(sock.setReuseAddr(true));

  int optval = 0;
  socklen_t optlen = sizeof(optval);
  ::getsockopt(sock.fd(), SOL_SOCKET, SO_REUSEADDR, &optval, &optlen);
  EXPECT_NE(optval, 0); // Verify option is enabled (macOS returns 4, not 1)
}

TEST_F(SocketTest, SetReusePort) {
  auto sock = Socket::createTcpSocket();
  EXPECT_NO_THROW(sock.setReusePort(true));

  int optval = 0;
  socklen_t optlen = sizeof(optval);
  ::getsockopt(sock.fd(), SOL_SOCKET, SO_REUSEPORT, &optval, &optlen);
  EXPECT_NE(optval,
            0); // Verify option is enabled (macOS returns 512, not 1)
}

TEST_F(SocketTest, BindAndListen) {
  auto sock = Socket::createTcpSocket();
  sock.setReuseAddr(true);

  InetAddress addr(9999);
  EXPECT_NO_THROW(sock.bind(addr));
  EXPECT_NO_THROW(sock.listen());
}

TEST_F(SocketTest, AcceptNonBlocking) {
  auto serverSock = Socket::createTcpSocket();
  serverSock.setReuseAddr(true);
  serverSock.setNonBlocking();

  InetAddress serverAddr(8888);
  serverSock.bind(serverAddr);
  serverSock.listen();

  // Try to accept without any client (should return -1 with EAGAIN)
  InetAddress peerAddr;
  int connfd = serverSock.accept(&peerAddr);
  EXPECT_EQ(connfd, -1);
  EXPECT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
}

TEST_F(SocketTest, ConnectAndAccept) {
  auto serverSock = Socket::createTcpSocket();
  serverSock.setReuseAddr(true);

  InetAddress serverAddr(7777);
  serverSock.bind(serverAddr);
  serverSock.listen();

  // Connect from another thread
  std::thread clientThread([&]() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int clientFd = ::socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_GE(clientFd, 0);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(7777);

    int ret =
        ::connect(clientFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    EXPECT_EQ(ret, 0);
    ::close(clientFd);
  });

  // Accept the connection
  InetAddress peerAddr;
  int connfd = serverSock.accept(&peerAddr);
  EXPECT_GE(connfd, 0);
  EXPECT_EQ(peerAddr.toIp(), "127.0.0.1");

  ::close(connfd);
  clientThread.join();
}

TEST_F(SocketTest, ShutdownWrite) {
  auto sock = Socket::createTcpSocket();
  EXPECT_NO_THROW(sock.shutdownWrite());
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
