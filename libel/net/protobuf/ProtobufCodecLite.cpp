//
// Created by kaymind on 2021/1/9.
//

#include "libel/net/protobuf/ProtobufCodecLite.h"

#include "libel/base/logging.h"
#include "libel/net/tcp_connection.h"
#include "libel/net/Endian.h"
#include "libel/net/protorpc/google-inl.h"

#include <google/protobuf/message.h>
#include <zlib.h>

using namespace Libel;
using namespace Libel::net;

void ProtobufCodecLite::send(const TcpConnectionPtr &conn, const ::google::protobuf::Message &message) {
  Libel::net::Buffer buffer;
  fillEmptyBuffer(&buffer, message);
  conn->send(&buffer);
}

void ProtobufCodecLite::fillEmptyBuffer(Libel::net::Buffer *buffer, const google::protobuf::Message &message) {
  assert(buffer->readableBytes() == 0);
  buffer->append(tag_);

  int32_t byte_size = serializeToBuffer(message, buffer);

  int32_t checkSum = checksum(buffer->peek(), static_cast<int>(buffer->readableBytes()));
  buffer->appendInt32(checkSum);
  assert(buffer->readableBytes() == tag_.size() + static_cast<uint32_t>(byte_size) + kChecksumLen);
  (void) byte_size;
  int32_t len = sockets::hostToNetwork32(static_cast<int32_t>(buffer->readableBytes()));
  buffer->prepend(&len, sizeof len);
}

void ProtobufCodecLite::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, TimeStamp receiveTime) {
  while (buffer->readableBytes() >= static_cast<uint32_t>(kMinMessageLen_ + kHeaderLen)) {
    const int32_t len = buffer->peekInt32();
    if (len > kMaxMessageLen || len < kMinMessageLen_) {
      errorCallback_(conn, buffer, receiveTime, kInvalidLength);
      break;
    } else if (buffer->readableBytes() >= implicit_cast<size_t>(kHeaderLen + len)) {
      if (rawMessageCallback_ && !rawMessageCallback_(conn, std::string(buffer->peek(), kHeaderLen + len), receiveTime)) {
        buffer->retrieve(kHeaderLen + len);
        continue;
      }
      MessagePtr message(prototype_->New());
      ErrorCode errorCode = parse(buffer->peek() + kHeaderLen, len, message.get());
      if (errorCode == kNoError) {
        messageCallback_(conn, message, receiveTime);
        buffer->retrieve(kHeaderLen + len);
      } else {
        errorCallback_(conn, buffer, receiveTime, errorCode);
        break;
      }
    } else {
      break;
    }
  }
}

bool ProtobufCodecLite::parseFromBuffer(std::string buffer, google::protobuf::Message *message) {
  return message->ParseFromArray(buffer.data(), static_cast<int>(buffer.size()));
}

int ProtobufCodecLite::serializeToBuffer(const google::protobuf::Message &message, Buffer *buffer) {
  GOOGLE_CHECK(message.IsInitialized()) << InitializationErrorMessage("serialize", message);

  /**
 * 'ByteSize()' of message is deprecated in Protocol Buffers v3.4.0 firstly. But, till to v3.11.0, it just getting start to be marked by '__attribute__((deprecated()))'.
 * So, here, v3.9.2 is selected as maximum version using 'ByteSize()' to avoid potential effect for previous muduo code/projects as far as possible.
 * Note: All information above just INFER from
 * 1) https://github.com/protocolbuffers/protobuf/releases/tag/v3.4.0
 * 2) MACRO in file 'include/google/protobuf/port_def.inc'. eg. '#define PROTOBUF_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))'.
 * In addition, usage of 'ToIntSize()' comes from Impl of ByteSize() in new version's Protocol Buffers.
 */

#if GOOGLE_PROTOBUF_VERSION > 3009002
  int byte_size = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
  int byte_size = message.ByteSize();
#endif
  buffer->ensureWriteableBytes(byte_size + kChecksumLen);
  auto start = reinterpret_cast<uint8_t*>(buffer->beginWrite());
  auto end = message.SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size)
  {
#if GOOGLE_PROTOBUF_VERSION > 3009002
    ByteSizeConsistencyError(byte_size, google::protobuf::internal::ToIntSize(message.ByteSizeLong()), static_cast<int>(end - start));
#else
    ByteSizeConsistencyError(byte_size, message.ByteSize(), static_cast<int>(end - start));
#endif
  }
  buffer->hasWritten(byte_size);
  return byte_size;
}

namespace {
const std::string kNoErrorStr = "NoError";
const std::string kInvalidLengthStr = "InvalidLength";
const std::string kCheckSumErrorStr = "CheckSumError";
const std::string kInvalidNameLenStr = "InvalidNameLen";
const std::string kUnknownMessageTypeStr = "UnknownMessageType";
const std::string kParseErrorStr = "ParseError";
const std::string kUnknownErrorStr = "UnknownError";
}

const std::string& ProtobufCodecLite::errorCodeToString(ErrorCode errorCode)
{
  switch (errorCode)
  {
    case kNoError:
      return kNoErrorStr;
    case kInvalidLength:
      return kInvalidLengthStr;
    case kCheckSumError:
      return kCheckSumErrorStr;
    case kInvalidNameLen:
      return kInvalidNameLenStr;
    case kUnknownMessageType:
      return kUnknownMessageTypeStr;
    case kParseError:
      return kParseErrorStr;
    default:
      return kUnknownErrorStr;
  }
}

void ProtobufCodecLite::defaultErrorCallback(const TcpConnectionPtr &conn, Buffer *buffer, TimeStamp, ErrorCode errorCode) {
  LOG_ERROR << "ProtobufCodecLite::defaultErrorCallback - " << errorCodeToString(errorCode);
  if (conn && conn->connected())
    conn->shutdown();
}

int32_t ProtobufCodecLite::asInt32(const char *buffer) {
  int32_t be32 = 0;
  ::memcpy(&be32, buffer, sizeof be32);
  return sockets::networkToHost32(be32);
}

int32_t ProtobufCodecLite::checksum(const void *buffer, int len) {
  return static_cast<int32_t>(::adler32(1, static_cast<const Bytef*>(buffer), len));
}

bool ProtobufCodecLite::validateChecksum(const char *buffer, int len) {
  int32_t expectedCheckSum = asInt32(buffer + len - kChecksumLen);
  int32_t checkSum = checksum(buffer, len - kChecksumLen);
  return checkSum == expectedCheckSum;
}

ProtobufCodecLite::ErrorCode ProtobufCodecLite::parse(const char *buf, int len, ::google::protobuf::Message *message) {
  ErrorCode errorCode = kNoError;

  if (validateChecksum(buf, len)) {
    if (memcmp(buf, tag_.data(), tag_.size()) == 0) {
      // parse from buffer
      const char* data = buf + tag_.size();
      int32_t dataLen = len - kChecksumLen - static_cast<int>(tag_.size());
      if (parseFromBuffer(std::string(data, dataLen), message)) {
        errorCode = kNoError;
      } else {
        errorCode = kParseError;
      }
    } else {
      errorCode = kUnknownMessageType;
    }
  } else {
    errorCode = kCheckSumError;
  }
  return errorCode;
}


















