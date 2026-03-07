/**
 * @file coro_echo_server.cc
 * @brief Coroutine-style Echo Server
 *
 * Demonstrates the full hayai coroutine stack:
 *   AsyncServer → accept()
 *   AsyncConnection → recv() / send()
 *   Task<void> → coroutine type
 *
 * This server reads data from each client and echoes it back.
 * Each connection is handled by a dedicated coroutine that looks
 * and reads like synchronous code, but is fully asynchronous.
 *
 * Usage:
 *   ./coro_echo_server [port]
 *
 * Test with netcat:
 *   nc 127.0.0.1 8080
 */

#include "hayai/coro/AsyncConnection.h"
#include "hayai/coro/AsyncServer.h"
#include "hayai/coro/Task.h"
#include "hayai/coro/spawn.h"
#include "hayai/net/EventLoop.h"
#include "hayai/net/InetAddress.h"
#include <atomic>
#include <csignal>
#include <iostream>

using namespace hayai;
using namespace hayai::coro;

// Global flag and loop pointer for graceful shutdown
static std::atomic<bool> running{true};
static EventLoop *g_loop = nullptr;

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
    running.store(false);
    if (g_loop)
      g_loop->quit(); // unblocks loop.loop()
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

  // Expose loop for signal handler, then run (blocks until loop.quit())
  g_loop = &loop;
  loop.loop();

  std::cout << "\n[Server] Shutdown complete.\n";

  return 0;
}