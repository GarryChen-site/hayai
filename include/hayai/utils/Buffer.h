#pragma once

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

namespace hayai {

class Buffer {
public:
  static constexpr size_t kInitialSize = 1024;
  static constexpr size_t kCheapPrepend = 8;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend) {}

  // Sizes
  [[nodiscard]] size_t readableBytes() const {
    return writerIndex_ - readerIndex_;
  }

  [[nodiscard]] size_t writableBytes() const {
    return buffer_.size() - readerIndex_;
  }

  [[nodiscard]] size_t prependableBytes() const { return readerIndex_; }

  // Read access
  [[nodiscard]] const char *peek() const { return begin() + readerIndex_; }

  // Find operations
  [[nodiscard]] const char *findCRLF() const {
    static constexpr char kCRLF[] = "\r\n";
    const char *crlf = std::search(peek(), peek() + readableBytes(),
                                   std::begin(kCRLF), std::end(kCRLF) - 1);
    return crlf == (peek() + readableBytes()) ? nullptr : crlf;
  }

  // Write access
  void append(const char *data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
  }

  void append(std::string_view str) { append(str.data(), str.size()); }

  // Read from socket
  ssize_t readFd(int fd, int *savedErrno);

  void retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
      readerIndex_ += len;
    } else {
      retrieveAll();
    }
  }

  void retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  [[nodiscard]] std::string retrieveAllAsString() {
    std::string result(begin() + readerIndex_, readableBytes());
    retrieveAll();
    return result;
  }

  [[nodiscard]] std::string retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string result(begin() + readerIndex_, len);
    retrieve(len);
    return result;
  }

  // Memory management
  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
  }

  void shrink(size_t reserve) {
    Buffer other;
    other.ensureWritableBytes(readableBytes() + reserve);
    other.append(peek(), readableBytes());
    swap(other);
  }

  void swap(Buffer &ths) noexcept {
    buffer_.swap(ths.buffer_);
    std::swap(readerIndex_, ths.readerIndex_);
    std::swap(writerIndex_, ths.writerIndex_);
  }

private:
  char *begin() { return buffer_.data(); }

  const char *begin() const { return buffer_.data(); }

  char *beginWrite() { return begin() + writerIndex_; }

  const char *beginWrite() const { return begin() + writerIndex_; }

  void makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      buffer_.resize(writerIndex_ + len);
    } else {
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_, begin() + writerIndex_,
                begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};
} // namespace hayai