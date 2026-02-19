#include "hayai/coro/AsyncServer.h"

#include <csignal>
#include <exception>
#include <iostream>
#include <string>

using namespace hayai;
using namespace hayai::coro;

// Global flag for graceful shutdown
static std::atomic<bool> running{true};

Task<void> handleClient(AsyncConnection conn) {
  const std::string peerAddr = conn.peerAddr().toIpPort();
  std::cout << "[+] Client connected: " << peerAddr << std::endl;

  try {
    while (conn.connected()) {
      Buffer buf = co_await conn.recv();
      if (buf.readableBytes() == 0) {
        break;
      }

      std::string_view data{buf.peek(), buf.readableBytes()};
      std::cout << "[" << peerAddr << "] recv " << buf.readableBytes()
                << " bytes\n";

      co_await conn.send(std::string(data));
      std::cout << "[" << peerAddr << "] sent " << buf.readableBytes()
                << " bytes\n";
    }
  } catch (const std::exception &e) {
    std::cerr << "[" << peerAddr << "] error: " << e.what() << "\n";
  }

  std::cout << "[-] Client disconnected: " << peerAddr << std::endl;
  co_return;
}

Task<void> runServer(AsyncServer &server) {

  std::cout << "[Server] Accepting connections...\n";

  while (running.load()) {
    try {
      AsyncConnection conn = co_await server.accept();

      handleClient(std::move(conn)).detach();
    } catch (const std::exception &e) {
      if (running.load()) {
        std::cerr << "[Server] Accept error: " << e.what() << "\n";
      }
    }
  }
  std::cout << "[Server] Accept loop exited.\n";
  co_return;
}

int main(int argc, char *argv[]) {
  uint16_t port = 9996;
  if (argc > 1) {
    port = static_cast<uint16_t>(std::stoi(argv[1]));
  }

  std::signal(SIGINT, [](int) {
    std::cout << "\n[Server] Shutting down...\n";
    running.store(false);
  });

  EventLoop loop;
  InetAddress listenAddr(port);

  AsyncServer server(&loop, listenAddr, "CoroEchoServer");
  server.setIoLoopNum(2);

  std::cout << "=== Hayai Coroutine Echo Server ===\n";
  std::cout << "Listening on port " << port << "\n";
  std::cout << "Press Ctrl+C to stop\n\n";

  server.start();

  runServer(server).detach();

  loop.loop();

  return 0;
}