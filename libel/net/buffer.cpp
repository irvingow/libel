//
// Created by kaymind on 2020/12/4.
//

#include "libel/net/buffer.h"
#include "libel/net/sockets_ops.h"

#include <cerrno>
#include <sys/uio.h>

using namespace Libel;
using namespace Libel::net;

const char Buffer::kCRLF[] = "\r\n";

/// A buffer class modeled after muduo::net::Buffer
///
/// @code
/// +-------------------+------------------+------------------+
///// | prependable bytes |  readable bytes  |  writable bytes  |
///// |                   |     (CONTENT)    |                  |
///// +-------------------+------------------+------------------+
///// |                   |                  |                  |
///// 0      <=      readerIndex   <=   writerIndex    <=     size
/// begin()
///// @endcode
ssize_t Buffer::readFd(int fd, int *savedErrno) {
  char extraBuf[65536];
  struct iovec vec[2];
  auto writable = writableBytes();
  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extraBuf;
  vec[1].iov_len = sizeof(extraBuf);
  // when there is enough space in buffer, data wouldn't be read into extraBuf.
  // when extraBuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < sizeof(extraBuf)) ? 2 : 1;
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0) {
    *savedErrno = errno;
  } else if (implicit_cast<size_t>(n) <= writable) {
    writerIndex_ += n;
  } else {
    writerIndex_ = buffer_.size();
    append(extraBuf, n - writable);
  }
  // actually if the data is very very big, buffer will not
  // fully read them all.
  return n;
}