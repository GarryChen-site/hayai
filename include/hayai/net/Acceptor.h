
#include "hayai/net/Channel.h"
#include "hayai/net/InetAddress.h"
#include "hayai/net/Socket.h"
#include "hayai/utils/NonCopyable.h"
#include <functional>

namespace hayai {
class EventLoop;

class Acceptor : NonCopyable {
public:
  using NewConnectionCallback =
      std::function<void(int sockfd, const InetAddress &)>;

  Acceptor(EventLoop *loop, const InetAddress &listenAddr,
           bool reuseport = true);
  ~Acceptor();

  void setNewConnectionCallback(NewConnectionCallback cb) {
    newConnectionCallback_ = std::move(cb);
  }
  void listen();

  [[nodiscard]] bool listening() const { return listening_; }
  [[nodiscard]] const InetAddress &address() const { return listenAddr_; }

private:
  void handleRead();

  EventLoop *loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  InetAddress listenAddr_;
  NewConnectionCallback newConnectionCallback_;
  bool listening_{false};
};
} // namespace hayai