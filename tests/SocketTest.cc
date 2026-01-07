
#include "hayai/net/Socket.h"
#include "hayai/net/InetAddress.h"
#include <gtest/gtest.h>

TEST(SocketTest, CtrateAndBind) {
  auto sock = hayai::Socket::createTcpSocket();
  EXPECT_GE(sock.fd(), 0);

  hayai::InetAddress addr(9999);
  EXPECT_NO_THROW(sock.bind(addr));
}

TEST(SocketTest, ListenAndAccept) {
  auto server = hayai::Socket::createTcpSocket();
  server.setReuseAddr();

  hayai::InetAddress serverAddr(8888);
  server.bind(serverAddr);
  server.listen();
  server.setNonBlocking();

  hayai::InetAddress peerAddr;
  int connfd = server.accept(&peerAddr);
  EXPECT_EQ(connfd, -1);
  EXPECT_EQ(errno, EAGAIN);
}