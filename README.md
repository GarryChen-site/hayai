# Hayai — Modern C++ TCP Server with Coroutine Support

A learning project demonstrating how to build a callback-based TCP server (Reactor pattern) and then layer C++20 coroutines on top as a **non-invasive wrapper**.

> **Goal**: understand how async I/O frameworks like Trantor/Muduo work, and how coroutines can make async code read like synchronous code.

---

## Quick Start

```bash
cmake -S . -B build && cmake --build build

# Run the coroutine echo server
./build/coro_echo_server 8080

# Test it interactively (in another terminal)
#   type something, press Enter → server echoes it back
#   Ctrl+D to disconnect, Ctrl+C to stop the server
nc 127.0.0.1 8080
```

> **Why not `echo "hello" | nc`?**  
> Piping immediately closes nc's stdin after sending — the server echoes correctly
> but nc exits before you see the reply, and the server logs `client disconnected`.
> Use interactive `nc` (no pipe) to see the full exchange.

---

## Architecture

Hayai is built in two layers:

```
┌─────────────────────────────────────────┐
│       Coroutine Layer (coro/)           │  ← This project's addition
│  Task<T>  AsyncConnection  AsyncServer  │
│              spawn()                    │
└───────────────────┬─────────────────────┘
                    │  wraps (non-invasive)
┌───────────────────▼─────────────────────┐
│       Callback Layer (net/)             │  ← Classic Reactor
│  EventLoop  TcpServer  TcpConnection    │
│  Acceptor   Channel    Poller(kqueue)   │
└─────────────────────────────────────────┘
```

---

### The Reactor Pattern (Callback Layer)

The Reactor is the foundation. All I/O is **non-blocking** — instead of waiting for data, you register a callback and the event loop calls it when the OS says data is ready.

```
   Client A ──────────────────────────────────────────────────────┐
   Client B ──────────────┐                                        │
   Client C ──┐           │                                        │
              │           │                                        │
              ▼           ▼                                        ▼
         ┌─────────── Acceptor ────────────┐              TcpConnection A
         │    listen() on port 8080        │              TcpConnection B
         │    accept() → new fd per client │              TcpConnection C
         └────────────────────────────────┘
                          │ newConnection callback
                          ▼
              ┌──────── TcpServer ──────────┐
              │  manages connection map     │
              │  round-robins I/O threads   │
              └────────────┬───────────────┘
                           │ assigns each conn to an I/O thread
                           ▼
         ┌──────────── EventLoop ──────────────────┐
         │                                         │
         │   loop() {                              │
         │     while (!quit) {                     │
         │       fds = Poller.poll()   ← kqueue    │
         │       for fd in fds:                    │
         │         Channel(fd).handleEvent()       │
         │         └─► fires onRead / onWrite /    │
         │             onClose callback            │
         │       doPendingFunctors()               │
         │     }                                   │
         │   }                                     │
         └─────────────────────────────────────────┘
```

**Key ideas:**
- **One EventLoop = one thread** (thread-local; never shared)
- **`Channel`** owns one fd and maps OS events (`EVFILT_READ`, `EVFILT_WRITE`) to C++ callbacks
- **`Poller`** (kqueue on macOS) is the OS primitive — it blocks until at least one fd is ready
- **`TcpServer`** uses an `EventLoopThreadPool` so accept happens on the main loop while I/O happens on N worker loops

The classical callback style looks like:
```cpp
server.setConnectionCallback([](const TcpConnectionPtr& conn) {
    if (conn->connected())
        conn->send("hello\n");
});
server.setMessageCallback([](const TcpConnectionPtr& conn, Buffer* buf) {
    conn->send(buf->retrieveAllAsString());   // echo
});
```

---

### Coroutine Layer — The Wrapper

The coroutine layer **wraps** the callback layer without modifying it. Every `co_await` is backed by a callback internally:

| Component | Role |
|-----------|------|
| `Task<T>` | The coroutine return type (like `std::future` but composable) |
| `AsyncConnection` | Wraps `TcpConnection`; provides `co_await recv()` / `co_await send()` |
| `AsyncServer` | Wraps `TcpServer`; provides `co_await accept()` |
| `spawn(loop, task)` | Fire-and-forget launcher with proper frame ownership |

---

## The Coroutine API

### `Task<T>` — Coroutine Return Type

```cpp
#include "hayai/coro/Task.h"

// A coroutine that returns a value
Task<int> computeAnswer() {
    co_return 42;
}

// A coroutine that returns nothing
Task<void> doWork() {
    int answer = co_await computeAnswer();   // chain coroutines
    co_return;
}
```

`Task<T>` uses **lazy evaluation** — it doesn't start until `co_await`-ed or explicitly resumed.

---

### `AsyncConnection` — Awaitable I/O

```cpp
#include "hayai/coro/AsyncConnection.h"

Task<void> handleClient(AsyncConnection conn) {
    while (conn.connected()) {
        Buffer data = co_await conn.recv();          // suspend until data arrives
        if (data.readableBytes() == 0) break;        // EOF

        co_await conn.send(data.retrieveAllAsString()); // suspend until sent
    }
}
```

**How it works**: `recv()` registers a callback on `TcpConnection`. When data arrives, the callback resumes the suspended coroutine via `EventLoop::queueInLoop`.

---

### `AsyncServer` — Awaitable Accept

```cpp
#include "hayai/coro/AsyncServer.h"

Task<void> acceptLoop(EventLoop& loop, AsyncServer& server) {
    while (true) {
        AsyncConnection conn = co_await server.accept();  // suspend until client connects
        spawn(&loop, handleClient(std::move(conn)));       // spawn per-client coroutine
    }
}
```

Internally, `AsyncServer` queues incoming connections and resumes the waiting `co_await accept()` on arrival.

---

### `spawn()` — Fire and Forget

```cpp
#include "hayai/coro/spawn.h"

// Starts the coroutine on the EventLoop and owns its lifetime.
// Frame is destroyed automatically when the coroutine completes.
spawn(&loop, myCoroutine());

// Instead of the leaky:
myCoroutine().detach();   // ← leaks frame if the coroutine suspends
```

---

## Complete Example — Echo Server

```cpp
#include "hayai/coro/AsyncServer.h"
#include "hayai/coro/spawn.h"
#include "hayai/net/EventLoop.h"

using namespace hayai;
using namespace hayai::coro;

Task<void> handleClient(AsyncConnection conn) {
    try {
        while (conn.connected()) {
            Buffer buf = co_await conn.recv();
            if (buf.readableBytes() == 0) break;
            co_await conn.send(buf.retrieveAllAsString());
        }
    } catch (const std::exception& e) { /* connection reset etc */ }
}

Task<void> runServer(EventLoop& loop, AsyncServer& server) {
    while (true) {
        AsyncConnection conn = co_await server.accept();
        spawn(&loop, handleClient(std::move(conn)));
    }
}

int main() {
    EventLoop loop;
    InetAddress addr(8080);

    AsyncServer server(&loop, addr, "EchoServer");
    server.setIoLoopNum(2);
    server.start();

    spawn(&loop, runServer(loop, server));
    loop.loop();
}
```

Full source: [`examples/coro_echo_server.cc`](examples/coro_echo_server.cc)

---

## How Callbacks Become `co_await`

The bridge pattern used throughout the coroutine layer:

```
co_await conn.recv()
│
├── await_ready()  → false (data not yet arrived)
│
├── await_suspend(handle)
│       └── stores handle in AsyncConnection
│           (registers callback on TcpConnection)
│
│   ... EventLoop runs, data arrives ...
│
│   TcpConnection fires messageCallback_
│       └── AsyncConnection::onMessage()
│               └── loop->queueInLoop([handle] { handle.resume(); })
│
└── await_resume()  → returns Buffer with received data
```

The coroutine is suspended (non-blocking) and resumed only when data is ready. The EventLoop thread is free to handle other connections in between.

---

## Project Structure

```
hayai/
├── CMakeLists.txt
├
│
├── include/hayai/
│   ├── net/                        # Callback layer — Reactor components
│   │   ├── EventLoop.h             # Reactor core: poll loop + task queue
│   │   ├── Channel.h               # fd → read/write/close callbacks
│   │   ├── Poller.h                # kqueue wrapper (macOS)
│   │   ├── Acceptor.h              # listen() + accept() for new clients
│   │   ├── Socket.h                # RAII fd wrapper
│   │   ├── InetAddress.h           # IP:port address type
│   │   ├── TcpConnection.h         # One live TCP connection
│   │   ├── TcpServer.h             # High-level server (composes above)
│   │   ├── EventLoopThread.h       # Runs an EventLoop on its own thread
│   │   └── EventLoopThreadPool.h   # Pool of EventLoopThreads (I/O workers)
│   ├── coro/                       # Coroutine layer — wraps the net/ layer
│   │   ├── Task.h                  # Coroutine return type Task<T>
│   │   ├── Awaiter.h               # Base awaiter utilities
│   │   ├── AsyncConnection.h       # co_await recv() / send()
│   │   ├── AsyncServer.h           # co_await accept()
│   │   └── spawn.h                 # fire-and-forget coroutine launcher
│   └── utils/
│       ├── Buffer.h                # Growable I/O buffer (header)
│       └── NonCopyable.h           # Delete copy ctor/assign mixin
│
├── src/
│   ├── net/                        # Implementations of the net/ headers
│   │   ├── EventLoop.cc
│   │   ├── Channel.cc
│   │   ├── Poller.cc
│   │   ├── Acceptor.cc
│   │   ├── Socket.cc
│   │   ├── InetAddress.cc
│   │   ├── TcpConnection.cc
│   │   ├── TcpServer.cc
│   │   ├── EventLoopThread.cc
│   │   └── EventLoopThreadPool.cc
│   ├── coro/                       # Implementations of the coro/ headers
│   │   ├── AsyncConnection.cc
│   │   └── AsyncServer.cc
│   └── utils/
│       └── Buffer.cc
│
├── examples/
│   ├── echo_server.cc              # Callback-style echo server
│   ├── coro_echo_server.cc         # Coroutine-style echo server ← main demo
│   └── coro_basic_demo.cc          # Minimal Task<T> usage demo
│
└── tests/
    ├── InetAddressTest.cc
    ├── SocketTest.cc
    ├── BufferTest.cc
    ├── TcpConnectionTest.cc
    ├── TcpServerTest.cc
    ├── AcceptorTest.cc
    ├── EventLoopTest.cc
    ├── EventLoopThreadPoolTest.cc
    ├── EchoServerIntegrationTest.cc
    ├── CoroTaskTest.cc
    ├── CoroConnectionTest.cc
    ├── CoroServerTest.cc
    ├── CoroSpawnTest.cc
    └── coro_echo_smoke_test.py     # End-to-end test for coro_echo_server
```

---

## Building

**Requirements**: clang with C++20, CMake 3.14+, macOS (kqueue)

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build          # run all tests (10/10 should pass)
```

---

## Key Design Decisions

**1. Non-invasive wrapper**: The coroutine layer adds zero changes to `TcpServer`, `TcpConnection`, or `EventLoop` (except `spawn()`). You can use the callback API and coroutine API side-by-side.

**2. Eager callback binding**: `AsyncServer::onConnection` wraps the raw `TcpConnectionPtr` in an `AsyncConnection` immediately at accept time, before any data can arrive — avoiding a race where an empty `messageCallback_` was called.

**3. Move-after-move safety**: `AsyncConnection` is move-only. Its move constructor calls `bindCallbacks()` to re-register lambdas on the new `this`, preventing dangling captures.

**4. `spawn()` vs `detach()`**: `detach()` leaks the coroutine frame if the coroutine suspends. `spawn()` stores the handle in `EventLoop::spawnedTasks_` and the `FinalAwaiter` calls `cleanupSpawnedTask()` to destroy it on completion.
