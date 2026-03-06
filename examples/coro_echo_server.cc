#include "hayai/coro/AsyncServer.h"

#include "hayai/coro/spawn.h"
#include <csignal>
#include <exception>
#include <iostream>
#include <string>

using namespace hayai;
using namespace hayai::coro;

// Global flag for graceful shutdown
static std::atomic<bool> running{true};

// ── Per-connection coroutine ───────────────────────────────────────────────

/**
 * Handles a single client connection as a coroutine.
 * Reads until the connection closes, echoing each chunk.
 */
Task<void> handleClient(AsyncConnection conn) {
  const std::string peerAddr = conn.peerAddr().toIpPort();
  std::cout << "[+] Client connected: " << peerAddr << "\n";

  try {
    while (conn.connected()) {
      // Suspend until data arrives
      Buffer buf = co_await conn.recv();

      if (buf.readableBytes() == 0) {
        // EOF - client disconnected
        break;
      }

      std::string_view data{buf.peek(), buf.readableBytes()};
      std::cout << "[" << peerAddr << "] recv " << buf.readableBytes()
                << " bytes\n";

      // Echo it back - suspend until write completes
      co_await conn.send(std::string(data));

      std::cout << "[" << peerAddr << "] sent " << buf.readableBytes()
                << " bytes\n";
    }
  } catch (const std::exception &e) {
    std::cerr << "[" << peerAddr << "] error: " << e.what() << "\n";
  }

  std::cout << "[-] Client disconnected: " << peerAddr << "\n";
  co_return;
}

// ── Accept loop coroutine ──────────────────────────────────────────────────

/**
 * Main server accept loop as a coroutine.
 * Accepts connections indefinitely, spawning a handler for each.
 */
Task<void> runServer(EventLoop &loop, AsyncServer &server) {
  std::cout << "[Server] Accepting connections...\n";

  while (running.load()) {
    try {
      // Suspend until next client connects
      AsyncConnection conn = co_await server.accept();

      // Spawn per-connection coroutine with proper ownership
      spawn(&loop, handleClient(std::move(conn)));
    } catch (const std::exception &e) {
      if (running.load()) {
        std::cerr << "[Server] Accept error: " << e.what() << "\n";
      }
    }
  }

  std::cout << "[Server] Accept loop exited.\n";
  co_return;
}

// ── Main ───────────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
  uint16_t port = 8080;
  if (argc > 1) {
    port = static_cast<uint16_t>(std::stoi(argv[1]));
  }

  // Graceful shutdown on Ctrl+C
  std::signal(SIGINT, [](int) {
    std::cout << "\n[Server] Shutting down...\n";
    running.store(false);
  });

  EventLoop loop;
  InetAddress listenAddr(port);

  AsyncServer server(&loop, listenAddr, "CoroEchoServer");
  server.setIoLoopNum(2); // 2 I/O threads for connection handling

  std::cout << "=== Hayai Coroutine Echo Server ===\n";
  std::cout << "Listening on port " << port << "\n";
  std::cout << "Press Ctrl+C to stop\n\n";

  server.start();

  // Start the coroutine accept loop (using spawn for proper ownership)
  spawn(&loop, runServer(loop, server));

  // Run the event loop (blocks until loop.quit())
  loop.loop();

  return 0;
}