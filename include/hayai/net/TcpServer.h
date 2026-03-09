#include "hayai/net/EventLoopThreadPool.h"
#include "hayai/net/TcpConnection.h"
#include "hayai/utils/NonCopyable.h"
#include <functional>
#include <map>

namespace hayai {
class EventLoop;
class Acceptor;

/**
 * @brief TcpServer - High-level TCP server API
 * Integrates all components to create a complete multi-threaded network server:
 * - Acceptor: Accepts new connections
 * - EventLoopThreadPool: Distributes connections across I/O threads
 * - TcpConnection: Manages individual connections
 *
 * Connection Flow:
 * 1. Client connects → Acceptor accepts fd
 * 2. TcpServer::newConnection() creates TcpConnection
 * 3. Connection assigned to next I/O thread (round-robin)
 * 4. User callbacks invoked
 * 5. On close → removeConnection() cleans up
 */
class TcpServer : NonCopyable {
public:
  using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
  using MessageCallback =
      std::function<void(const TcpConnectionPtr &, Buffer *)>;
  using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;

  TcpServer(EventLoop *loop, const InetAddress &addr, std::string name);
  ~TcpServer();

  /**
   * @brief Start the server (begin accepting connections)
   *
   * Thread-safe. Can be called from any thread.
   * Idempotent - multiple calls are safe.
   */
  void start();

  void stop();

  void setIoLoopNum(size_t num);

  void setConnectionCallback(ConnectionCallback cb) {
    connectionCallback_ = std::move(cb);
  }

  void setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
  }

  void setWriteCompleteCallback(WriteCompleteCallback cb) {
    writeCompleteCallback_ = std::move(cb);
  }

  [[nodiscard]] const std::string &name() const { return name_; }

  [[nodiscard]] EventLoop *getLoop() const { return loop_; }

  [[nodiscard]] bool started() const { return started_.load(); }

private:
  void newConnection(int sockfd, const InetAddress &peerAddr);
  void removeConnection(const TcpConnectionPtr &conn);
  void removeConnectionInLoop(const TcpConnectionPtr &conn);

  EventLoop *loop_;
  std::string name_;

  std::unique_ptr<Acceptor> acceptor_;
  std::unique_ptr<EventLoopThreadPool> threadPool_;

  std::map<std::string, TcpConnectionPtr> connections_;
  int nextConnId_{1};

  std::atomic<bool> started_{false};

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
};
} // namespace hayai