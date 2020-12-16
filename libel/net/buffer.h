//
// Created by kaymind on 2020/12/4.
//

#ifndef LIBEL_BUFFER_H
#define LIBEL_BUFFER_H

#include "libel/net/Endian.h"
#include "libel/net/callbacks.h"

#include <algorithm>
#include <vector>

#include <cassert>
#include <cstring>
#include <iostream>

namespace Libel {

namespace net {

/// A buffer class modeled after muduo::net::Buffer
///
/// @code
/// +-------------------+------------------+------------------+
///// | prependable bytes |  readable bytes  |  writable bytes  |
///// |                   |     (CONTENT)    |                  |
///// +-------------------+------------------+------------------+
///// |                   |                  |                  |
///// 0      <=      readerIndex   <=   writerIndex    <=     size
///// @endcode
class Buffer {
 public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend) {}

  void swap(Buffer &rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }

  size_t readableBytes() const {
    return writerIndex_ - readerIndex_;
  }

  size_t writableBytes() const {
    return buffer_.size() - writerIndex_;
  }

  size_t prependableBytes() const {
    return readerIndex_;
  }

  const char* peek() const {
    return begin() + readerIndex_;
  }

  const char *findCRLF() const {
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  const char* findCRLF(const char* start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  const char* findEOL() const {
    const void* eol = memchr(peek(), '\n', readableBytes());
    return static_cast<const char*>(eol);
  }

  const char* findEOL(const char* start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const void *eol = memchr(start, '\n', beginWrite() - start);
    return static_cast<const char*>(eol);
  }

  void retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  void retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
      readerIndex_ += len;
    } else {
      // reset readerIndex_ and writerIndex_
      retrieveAll();
    }
  }

  void retrieveUntil(const char* end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  void retrieveInt64()
  {
    retrieve(sizeof(int64_t));
  }

  void retrieveInt32()
  {
    retrieve(sizeof(int32_t));
  }

  void retrieveInt16()
  {
    retrieve(sizeof(int16_t));
  }

  void retrieveInt8()
  {
    retrieve(sizeof(int8_t));
  }

  std::string retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  std::string retrieveAllAsString() {
    return retrieveAsString(readableBytes());
  }

  char *beginWrite() {
    return begin() + writerIndex_;
  }

  const char* beginWrite() const {
    return begin() + writerIndex_;
  }

  void hasWritten(size_t len) {
    assert(len <= writableBytes());
    writerIndex_ += len;
  }

  void unwrite(size_t len) {
    assert(len <= readableBytes());
    writerIndex_ -= len;
  }

  void ensureWriteableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  void append(const char* data, size_t len) {
    ensureWriteableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }

  void append(const void* data, size_t len) {
    append(static_cast<const char*>(data), len);
  }

  void append(const std::string &str) {
    append(str.data(), str.size());
  }

  ///
  /// append int64_t using network endian
  ///
  void appendInt64(int64_t val) {
    int64_t netEndianVal = sockets::hostToNetwork64(val);
    append(&netEndianVal, sizeof(netEndianVal));
  }

  ///
  /// append int32_t using network endian
  ///
  void appendInt32(int32_t val) {
    int32_t netEndianVal = sockets::hostToNetwork32(val);
    append(&netEndianVal, sizeof(netEndianVal));
  }

  ///
  /// append int16_t using network endian
  ///
  void appendInt16(int16_t val) {
    int16_t netEndianVal = sockets::hostToNetwork16(val);
    append(&netEndianVal, sizeof(netEndianVal));
  }

  void appendInt8(int8_t val) {
    append(&val, sizeof(val));
  }

  ///
  /// Peek int64_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int64_t)
  int64_t peekInt64() const {
    assert(readableBytes() >= sizeof(int64_t));
    int64_t val = 0;
    ::memcpy(&val, peek(), sizeof(val));
    return sockets::networkToHost64(val);
  }

  ///
  /// Peek int32_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int32_t peekInt32() const {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t val = 0;
    ::memcpy(&val, peek(), sizeof(val));
    return sockets::networkToHost32(val);
  }

  ///
  /// Peek int16_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int16_t)
  int16_t peekInt16() const {
    assert(readableBytes() >= sizeof(int16_t));
    int16_t val = 0;
    ::memcpy(&val, peek(), sizeof(val));
    return sockets::networkToHost16(val);
  }

  int8_t peekInt8() const {
    assert(readableBytes() >= sizeof(int8_t));
    int8_t x = *peek();
    return x;
  }

  ///
  /// Read int64_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int64_t)
  int64_t readInt64() {
    int64_t result = peekInt64();
    retrieveInt64();
    return result;
  }

  ///
  /// Read int32_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int32_t readInt32() {
    int32_t result = peekInt32();
    retrieveInt32();
    return result;
  }

  ///
  /// Read int16_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(intq16_t)
  int16_t readInt16() {
    int16_t result = peekInt16();
    retrieveInt16();
    return result;
  }

  int8_t readInt8() {
    int8_t result = peekInt8();
    retrieveInt8();
    return result;
  }

  /// caller should ensure that @func prependableBytes() >= len
  void prepend(const void* data, size_t len) {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    auto d = static_cast<const char*>(data);
    std::copy(d, d+len, begin()+readerIndex_);
  }

  ///
  /// Prepend int64_t using network endian
  ///
  void prependInt64(int64_t pre) {
    int64_t netEndianPre = sockets::hostToNetwork64(pre);
    prepend(&netEndianPre, sizeof(netEndianPre));
  }

  ///
  /// Prepend int32_t using network endian
  ///
  void prependInt32(int32_t x)
  {
    int32_t be32 = sockets::hostToNetwork32(x);
    prepend(&be32, sizeof be32);
  }

  ///
  /// Prepend int16_t using network endian
  ///
  void prependInt16(int16_t x)
  {
    int16_t be16 = sockets::hostToNetwork16(x);
    prepend(&be16, sizeof be16);
  }

  void prependInt8(int8_t x)
  {
    prepend(&x, sizeof x);
  }

  std::string toString() const {
    return std::string(peek(), static_cast<int>(readableBytes()));
  }

  void shrink(size_t reserve) {
    Buffer other;
    other.ensureWriteableBytes(readableBytes()+reserve);
    other.append(toString());
    swap(other);
  }

  size_t internalCapacity() const {
    return buffer_.capacity();
  }

  /// Read data directly into buffer
  ///
  /// implement with readv
  /// using stack space and readv to
  /// balance buffer capacity and efficiency
  ssize_t readFd(int fd, int* savedErrno);

private:

  char* begin() {
    return buffer_.data();
  }

  const char* begin() const {
    return buffer_.data();
  }

  void makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      // need a new buffer
      buffer_.resize(writerIndex_ + len);
    } else {
      // move readable data to the front, make space inside buffer
      assert(kCheapPrepend < readerIndex_);
      size_t readable = readableBytes();
      std::copy(begin()+readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
      assert(readable == readableBytes());
    }
  }

  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;

  static const char kCRLF[];
};

}  // namespace net
}  // namespace Libel

#endif  // LIBEL_BUFFER_H
