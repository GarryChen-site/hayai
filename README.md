
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