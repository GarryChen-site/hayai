#include "hayai/net/EventLoop.h"
#include "hayai/net/InetAddress.h"
#include "hayai/net/TcpConnection.h"
#include "hayai/net/TcpServer.h"
#include <csignal>
#include <iostream>

using namespace hayai;

EventLoop *g_loop = nullptr;

void signalHandler(int) {
  if (g_loop) {
    g_loop->quit();
  }
}

int main() {
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);

  EventLoop loop;
  g_loop = &loop;

  InetAddress listenAddr(9999);
  TcpServer server(&loop, listenAddr, "EchoServer");

  server.setConnectionCallback([](const TcpConnectionPtr &conn) {
    if (conn->connected()) {
      std::cout << "New connection from " << conn->peerAddress().toIpPort()
                << std::endl;
    } else {
      std::cout << "Connection closed from " << conn->peerAddress().toIpPort()
                << std::endl;
    }
  });

  server.setMessageCallback([](const TcpConnectionPtr &conn, Buffer *buf) {
    std::string msg = buf->retrieveAllAsString();
    std::cout << "Received " << msg.size() << " bytes from "
              << conn->peerAddress().toIpPort() << ": " << msg;

    // Echo back
    conn->send(msg);
  });

  // Use 4 I/O threads
  server.setIoLoopNum(4);

  std::cout << "Echo server starting on port 9999..." << std::endl;
  std::cout << "Using 4 I/O threads" << std::endl;
  std::cout << "Press Ctrl+C to stop" << std::endl;

  server.start();
  loop.loop();

  std::cout << "\nServer stopped." << std::endl;
  return 0;
}
