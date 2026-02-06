#pragma once

#include "hayai/net/InetAddress.h"
#include "hayai/utils/Buffer.h"
#include "hayai/utils/NonCopyable.h"
#include <functional>
#include <memory>
#include <string_view>

namespace hayai {
class EventLoop;
class Socket;
class Channel;

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class TcpConnection : NonCopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
  using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
  using MessageCallback =
      std::function<void(const TcpConnectionPtr &, Buffer *)>;
  using CloseCallback = std::function<void(const TcpConnectionPtr &)>;

  TcpConnection(EventLoop *loop, std::string name, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr);

  ~TcpConnection();

  // User APIs
  void send(std::string_view message);
  void shutdown();
  void forceClose();

  [[nodiscard]] bool connected() const { return state_ == State::Connected; }

  [[nodiscard]] const InetAddress &localAddress() const { return localAddr_; }

  [[nodiscard]] const InetAddress &peerAddress() const { return peerAddr_; }

  // Set callbacks
  void setConnectionCallback(ConnectionCallback cb) {
    connectionCallback_ = std::move(cb);
  }

  void setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
  }

  void setCloseCallback(CloseCallback cb) { closeCallback_ = std::move(cb); }

  void setWriteCompleteCallback(ConnectionCallback cb) {
    writeCompleteCallback_ = std::move(cb);
  }

  [[nodiscard]] const std::string &name() const { return name_; }
  [[nodiscard]] EventLoop *getLoop() const { return loop_; }

  // Called by TcpServer/Acceptor
  void connectEstablished();
  void connectDestroyed();

private:
  enum class State { Connecting, Connected, Disconnecting, Disconnected };

  void handleRead();
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(std::string_view message);
  void shutdownInLoop();

  EventLoop *loop_;
  std::string name_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  State state_{State::Connecting};

  InetAddress localAddr_;
  InetAddress peerAddr_;

  Buffer inputBuffer_;
  Buffer outputBuffer_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  ConnectionCallback writeCompleteCallback_;
  CloseCallback closeCallback_;
};

} // namespace hayai