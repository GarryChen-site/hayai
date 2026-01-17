#include "hayai/utils/NonCopyable.h"
#include <functional>
namespace hayai {
class EventLoop;

class Channel : NonCopyable {
public:
  using EventCallback = std::function<void()>;

  Channel(EventLoop *loop, int fd);
  ~Channel();

  // Dispatcher
  void handleEvent();

  // Tie: prevents the owner object (e.g., TcpConnection) from being destroyed
  // while we are executing its callbacks;
  void tie(const std::shared_ptr<void> &obj);

  void setReadCallback(EventCallback cb) { readCallback_ = std::move(cb); }
  void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

  void enableReading() {
    events_ |= kReadEvent;
    update();
  }

  void disableReading() {
    events_ &= ~kReadEvent;
    update();
  }

  void enableWriting() {
    events_ |= kWriteEvent;
    update();
  }

  void disableWriting() {
    events_ &= ~kWriteEvent;
    update();
  }
  void disableAll() {
    events_ = kNoneEvent;
    update();
  }

  [[nodiscard]] int fd() const { return fd_; }
  [[nodiscard]] int events() const { return events_; }
  void setRevents(int revt) { revents_ = revt; }
  [[nodiscard]] bool isNoneEvent() const { return events_ == kNoneEvent; }
  [[nodiscard]] bool isReading() const { return events_ & kReadEvent; }
  [[nodiscard]] bool isWriting() const { return events_ & kWriteEvent; }

  [[nodiscard]] int index() const { return index_; }
  void setIndex(int idx) { index_ = idx; }

  // Event constants (Bitmask style)
  static constexpr int kNoneEvent = 0;
  static constexpr int kReadEvent = 1;
  static constexpr int kWriteEvent = 2;
  static constexpr int kErrorEvent = 4;
  static constexpr int kCloseEvent = 8;

private:
  void update();
  void handleEventWithGuard();

  EventLoop *loop_;
  const int fd_;
  int events_{kNoneEvent};  // Events Poller should watch
  int revents_{kNoneEvent}; // Events actually occurred
  int index_{-1};

  std::weak_ptr<void> tie_; // The "guard" for obj lifetime
  bool tied_{false};

  EventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};

} // namespace hayai