
## What are C++20 Coroutines?
Coroutines allow functions to suspend and resume execution, making async code look synchronous.

### Traditional Callback Hell

```cpp
// Echo server with callbacks (current hayai approach)
void handleConnection(const TcpConnectionPtr& conn) {
    conn->setMessageCallback([](const TcpConnectionPtr& conn, Buffer* buf) {
        std::string msg(buf->peek(), buf->readableBytes());
        buf->retrieveAll();
        
        // Send response
        conn->send(msg);
        
        // If we need to do more async operations, we nest deeper...
        conn->setWriteCompleteCallback([conn]() {
            doSomethingElse(conn, [conn]() {
                // This gets messy fast!
            });
        });
    });
}

```

### With Coroutines

```cpp
// Echo server with coroutines - LOOKS SYNCHRONOUS!
Task<void> handleConnection(AsyncTcpConnection conn) {
    try {
        while (true) {
            // Suspends here until data arrives - looks like blocking!
            std::string msg = co_await conn.recv();
            
            // Send response - suspends until written
            co_await conn.send(msg);
            
            // Can continue with more operations naturally
            co_await doSomethingElse(conn);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

```

**Advantages**:
✅ Looks like synchronous code (easy to read!)
✅ Natural error handling (try/catch)
✅ Local variables for state
✅ Sequential flow, no nesting
✅ Compiler generates state machine

---

## How Coroutines Work (Simplified)

### The Magic: Suspend and Resume

```cpp
Task<std::string> readData(AsyncSocket& sock) {
    // When we hit co_await, the function SUSPENDS
    // Control returns to the event loop
    // When data arrives, the coroutine RESUMES
    std::string data = co_await sock.recv(1024);
    
    // This line runs LATER, after data arrives
    return data;
}
```

**Behind the scenes**:
1. Compiler transforms coroutine into a **state machine**
2. `co_await` suspends execution, saves state on heap
3. Event loop continues processing other events
4. When data ready, event loop **resumes** the coroutine
5. Execution continues from suspension point

**Key insight**: It's syntactic sugar for callbacks, but MUCH cleaner!

---

## Conclusion

**Coroutines are objectively better** for writing async code, but callbacks provide the foundation. Hayai's callback approach teaches the fundamentals that underlie all async I/O.