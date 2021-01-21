//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/codec/codec.h"

#include "libel/base/logging.h"
#include "libel/net/Endian.h"
#include "libel/net/protorpc/google-inl.h"

#include <google/protobuf/descriptor.h>

#include <zlib.h>  // adler32

using namespace Libel;
using namespace Libel::net;

void ProtobufCodec::fillEmptyBuffer(Libel::net::Buffer *buffer,
                                    const google::protobuf::Message &message) {
  assert(buffer->readableBytes() == 0);

  const std::string &typeName = message.GetTypeName();
  int32_t nameLen = static_cast<int32_t>(typeName.size() + 1);
  buffer->appendInt32(nameLen);
  buffer->append(typeName.c_str(), nameLen);

  GOOGLE_DCHECK(message.IsInitialized())
      << InitializationErrorMessage("serialize", message);

  /**
   * 'ByteSize()' of message is deprecated in Protocol Buffers v3.4.0 firstly.
   * But, till to v3.11.0, it just getting start to be marked by
   * '__attribute__((deprecated()))'. So, here, v3.9.2 is selected as maximum
   * version using 'ByteSize()' to avoid potential effect for previous muduo
   * code/projects as far as possible. Note: All information above just INFER
   * from 1) https://github.com/protocolbuffers/protobuf/releases/tag/v3.4.0 2)
   * MACRO in file 'include/google/protobuf/port_def.inc'. eg. '#define
   * PROTOBUF_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))'. In
   * addition, usage of 'ToIntSize()' comes from Impl of ByteSize() in new
   * version's Protocol Buffers.
   */

#if GOOGLE_PROTOBUF_VERSION > 3009002
  int byte_size = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
  int byte_size = message.ByteSize();
#endif
  buffer->ensureWriteableBytes(byte_size);

  auto start = reinterpret_cast<uint8_t *>(buffer->beginWrite());
  auto end = message.SerializeWithCachedSizesToArray(start);
  if (end - start != byte_size) {
#if GOOGLE_PROTOBUF_VERSION > 3009002
    ByteSizeConsistencyError(
        byte_size,
        google::protobuf::internal::ToIntSize(message.ByteSizeLong()),
        static_cast<int>(end - start));
#else
    ByteSizeConsistencyError(byte_size, message.ByteSize(),
                             static_cast<int>(end - start));
#endif
  }
  buffer->hasWritten(byte_size);

  auto checkSum = static_cast<int32_t>(
      ::adler32(1, reinterpret_cast<const Bytef *>(buffer->peek()),
                static_cast<uint>(buffer->readableBytes())));
  buffer->appendInt32(checkSum);
  assert(buffer->readableBytes() ==
         sizeof nameLen + nameLen + byte_size + sizeof checkSum);
  auto len =
      sockets::hostToNetwork32(static_cast<uint32_t>(buffer->readableBytes()));
  buffer->prepend(&len, sizeof len);
}

namespace {
const std::string kNoErrorStr = "NoError";
const std::string kInvalidLengthStr = "InvalidLength";
const std::string kCheckSumErrorStr = "CheckSumError";
const std::string kInvalidNameLenStr = "InvalidNameLen";
const std::string kUnknownMessageTypeStr = "UnknownMessageType";
const std::string kParseErrorStr = "ParseError";
const std::string kUnknownErrorStr = "UnknownError";
}  // namespace

const std::string &ProtobufCodec::errorCodeToString(ErrorCode errorCode) {
  switch (errorCode) {
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

void ProtobufCodec::defaultErrorCallback(
    const Libel::net::TcpConnectionPtr &connection, Libel::net::Buffer *buffer,
    Libel::TimeStamp timeStamp, ErrorCode errorCode) {
  LOG_ERROR << "ProtobufCodec::defaultErrorCallback - "
            << errorCodeToString(errorCode);
  if (connection && connection->connected()) {
    connection->shutdown();
  }
}

int32_t asInt32(const char *buf) {
  int32_t be32 = 0;
  ::memcpy(&be32, buf, sizeof(be32));
  return static_cast<int32_t>(sockets::networkToHost32(be32));
}

void ProtobufCodec::onMessage(const Libel::net::TcpConnectionPtr &conn,
                              Libel::net::Buffer *buffer,
                              Libel::TimeStamp receiveTime) {
  while (buffer->readableBytes() >= kMinMessageLen + kHeaderLen) {
    const auto len = buffer->peekInt32();
    if (len > static_cast<int32_t>(kMaxMessageLen) ||
        len < static_cast<int32_t>(kMinMessageLen)) {
      errorCallback_(conn, buffer, receiveTime, kInvalidLength);
      break;
    } else if (buffer->readableBytes() >=
               implicit_cast<size_t>(len + static_cast<int32_t>(kHeaderLen))) {
      auto errorCode = kNoError;
      MessagePtr message = parse(buffer->peek() + kHeaderLen, len, &errorCode);
      if (errorCode == kNoError && message) {
        messageCallback_(conn, message, receiveTime);
        buffer->retrieve(
            implicit_cast<size_t>(len + static_cast<int32_t>(kHeaderLen)));
      } else {
        errorCallback_(conn, buffer, receiveTime, errorCode);
        break;
      }
    } else {
      break;
    }
  }
}

google::protobuf::Message *ProtobufCodec::createMessage(
    const std::string &type_name) {
  google::protobuf::Message *message = nullptr;
  const google::protobuf::Descriptor *descriptor =
      google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
          type_name);
  if (descriptor) {
    const google::protobuf::Message *prototype =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(
            descriptor);
    if (prototype) {
      message = prototype->New();
    }
  }
  return message;
}

MessagePtr ProtobufCodec::parse(const char *buffer, int len,
                                ErrorCode *errorCode) {
  MessagePtr message;

  int32_t expectedCheckSum = asInt32(buffer + len - kHeaderLen);
  int32_t checkSum = static_cast<int32_t>(
      ::adler32(1, reinterpret_cast<const Bytef *>(buffer),
                static_cast<uint>(len - static_cast<int>(kHeaderLen))));
  if (expectedCheckSum == checkSum) {
    // get message type name
    int32_t nameLen = asInt32(buffer);
    if (nameLen >= 2 &&
        nameLen <= (len - 2 * static_cast<int32_t>(kHeaderLen))) {
      std::string typeName(buffer + kHeaderLen,
                           buffer + kHeaderLen + nameLen - 1);
      // create message object
      message.reset(createMessage(typeName));
      if (message) {
        const char *data = buffer + kHeaderLen + nameLen;
        int32_t dataLen = len - nameLen - 2 * static_cast<int32_t>(kHeaderLen);
        if (message->ParseFromArray(data, dataLen)) {
          *errorCode = kNoError;
        } else {
          *errorCode = kParseError;
        }
      } else {
        *errorCode = kUnknownMessageType;
      }
    } else {
      *errorCode = kInvalidNameLen;
    }
  } else {
    *errorCode = kCheckSumError;
  }
  return message;
}
