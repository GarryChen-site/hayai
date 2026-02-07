#include "hayai/net/EventLoop.h"
#include "hayai/net/InetAddress.h"
#include "hayai/net/TcpConnection.h"
#include "hayai/net/TcpServer.h"
#include <arpa/inet.h>
#include <chrono>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace hayai {
namespace test {

class EchoServerIntegrationTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(EchoServerIntegrationTest, BasicEcho) {
  // Server must be created and run in its own thread
  // because EventLoop must run in the thread that created it

  std::atomic<bool> serverReady{false};
  bool connectionEstablished = false;

  // Start server in a separate thread
  std::thread serverThread([&]() {
    EventLoop serverLoop;
    InetAddress serverAddr(19999);
    TcpServer server(&serverLoop, serverAddr, "TestEchoServer");

    server.setConnectionCallback([&](const TcpConnectionPtr &conn) {
      if (conn->connected()) {
        connectionEstablished = true;
      }
    });

    server.setMessageCallback([](const TcpConnectionPtr &conn, Buffer *buf) {
      // Echo back whatever we receive
      std::string msg = buf->retrieveAllAsString();
      conn->send(msg);
    });

    server.start();
    serverReady = true;

    serverLoop.loop(); // Run in the same thread
  });

  // Wait for server to start
  while (!serverReady) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Create client socket
  int clientFd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(clientFd, 0);

  // Connect to server
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(19999);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  int ret =
      connect(clientFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
  ASSERT_EQ(ret, 0) << "Failed to connect to server";

  // Give connection time to establish
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Send a message
  const char *testMsg = "Hello, Echo Server!";
  ssize_t sent = send(clientFd, testMsg, strlen(testMsg), 0);
  ASSERT_EQ(sent, static_cast<ssize_t>(strlen(testMsg)));

  // Receive echo
  char buffer[1024] = {0};
  ssize_t received = recv(clientFd, buffer, sizeof(buffer), 0);
  ASSERT_GT(received, 0);

  std::string echoedMsg(buffer, received);
  EXPECT_EQ(echoedMsg, std::string(testMsg));

  // Clean up
  close(clientFd);

  // The server thread is running loop() which won't exit naturally for this
  // test Since we can't easily quit the loop from here, just detach the
  // thread. In a real application, you'd use proper shutdown signaling
  serverThread.detach();

  EXPECT_TRUE(connectionEstablished);
}

TEST_F(EchoServerIntegrationTest, MultipleMessages) {
  std::atomic<bool> serverReady{false};

  // Start server in a separate thread
  std::thread serverThread([&]() {
    EventLoop serverLoop;
    InetAddress serverAddr(19998);
    TcpServer server(&serverLoop, serverAddr, "TestEchoServer2");

    server.setMessageCallback([](const TcpConnectionPtr &conn, Buffer *buf) {
      std::string msg = buf->retrieveAllAsString();
      conn->send(msg);
    });

    server.start();
    serverReady = true;

    serverLoop.loop();
  });

  // Wait for server
  while (!serverReady) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Create client
  int clientFd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(clientFd, 0);

  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(19998);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  ASSERT_EQ(
      connect(clientFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)), 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // Send multiple messages
  std::vector<std::string> messages = {"First", "Second", "Third"};

  for (const auto &msg : messages) {
    send(clientFd, msg.c_str(), msg.size(), 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    char buffer[1024] = {0};
    ssize_t received = recv(clientFd, buffer, sizeof(buffer), 0);
    ASSERT_GT(received, 0);

    std::string echoedMsg(buffer, received);
    EXPECT_EQ(echoedMsg, msg);
  }

  close(clientFd);

  // Detach server thread
  serverThread.detach();
}

} // namespace test
} // namespace hayai

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
