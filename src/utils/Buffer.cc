#include "hayai/utils/Buffer.h"
#include <errno.h>
#include <sys/uio.h> // For readv
#include <unistd.h>

namespace hayai {
ssize_t Buffer::readFd(int fd, int *savedErrno) {

  // Problem: We don't know how much data the kernel has for us.
  // - If we allocate too much: waste memory
  // - If we allocate too little: need multiple read() calls
  //
  // Solution: Use readv() with TWO buffers:
  // 1. Our main buffer's writable space
  // 2. A 64KB stack-allocated buffer as overflow

  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();

  vec[0].iov_base = beginWrite();
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof(extrabuf);

  const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
  const ssize_t n = ::readv(fd, vec, iovcnt);

  if (n < 0) {
    *savedErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    writerIndex_ += n;
  } else {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }

  return n;
}
} // namespace hayai