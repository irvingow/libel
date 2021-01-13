//
// Created by kaymind on 2021/1/9.
//

#ifndef LIBEL_PROTOBUFCODECLITE_H
#define LIBEL_PROTOBUFCODECLITE_H

#include "libel/base/noncopyable.h"
#include "libel/base/timestamp.h"
#include "libel/net/callbacks.h"

#include <memory>
#include <type_traits>

namespace google {

namespace protobuf {
class Message;
}
}  // namespace google

namespace Libel {

namespace net {

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

// wire format
// Field     Length  Content
//
// size      4-byte  M+N+4
// tag       M-byte could be "RPC0", etc.
// payload   N-byte
// checksum  4-byte adler32 of tag+payload
//
// This is an internal class, you should use ProtobufCodecT instead.
class ProtobufCodecLite : noncopyable {
 public:
  const static int kHeaderLen = sizeof(int32_t);
  const static int kChecksumLen = sizeof(int32_t);
  const static int kMaxMessageLen = 64 * 1024 * 1024;

  enum ErrorCode {
    kNoError = 0,
    kInvalidLength,
    kCheckSumError,
    kInvalidNameLen,
    kUnknownMessageType,
    kParseError,
  };

  using RawMessageCallback =
      std::function<bool(const TcpConnectionPtr&, std::string, TimeStamp)>;
  using ProtobufMessageCallback = std::function<void(
      const TcpConnectionPtr&, const MessagePtr&, TimeStamp)>;
  using ErrorCallback = std::function<void(const TcpConnectionPtr&, Buffer*,
                                           TimeStamp, ErrorCode)>;

  ProtobufCodecLite(
      const ::google::protobuf::Message* prototype, std::string tagArg,
      ProtobufMessageCallback protobufMessageCallback,
      RawMessageCallback rawMessageCallback = RawMessageCallback(),
      ErrorCallback errorCallback = ErrorCallback())
      : prototype_(prototype),
        tag_(std::move(tagArg)),
        messageCallback_(std::move(protobufMessageCallback)),
        rawMessageCallback_(std::move(rawMessageCallback)),
        errorCallback_(std::move(errorCallback)),
        kMinMessageLen_(static_cast<int>(tagArg.size()) + kChecksumLen) {}

  virtual ~ProtobufCodecLite() = default;

  const std::string& tag() const { return tag_; }

  void send(const TcpConnectionPtr& conn,
            const ::google::protobuf::Message& message);

  void onMessage(const TcpConnectionPtr& conn, Buffer* buffer,
                 TimeStamp receiveTime);

  virtual bool parseFromBuffer(std::string buffer,
                               google::protobuf::Message* message);

  virtual int serializeToBuffer(const google::protobuf::Message& message,
                                Buffer* buffer);

  static const std::string& errorCodeToString(ErrorCode errorCode);

  ErrorCode parse(const char* buf, int len,
                  ::google::protobuf::Message* message);

  void fillEmptyBuffer(Libel::net::Buffer* buffer,
                       const google::protobuf::Message& message);

  static int32_t checksum(const void* buffer, int len);

  static bool validateChecksum(const char* buffer, int len);

  static int32_t asInt32(const char* buffer);

  static void defaultErrorCallback(const TcpConnectionPtr&, Buffer*, TimeStamp,
                                   ErrorCode);

 private:
  const ::google::protobuf::Message* prototype_;
  const std::string tag_;
  ProtobufMessageCallback messageCallback_;
  RawMessageCallback rawMessageCallback_;
  ErrorCallback errorCallback_;
  const int kMinMessageLen_;
};

// TAG must be a variable with external linkage, not a string literal
template <typename MSG, const char* TAG, typename CODEC = ProtobufCodecLite>
class ProtobufCodecLiteT {
  static_assert(std::is_base_of<ProtobufCodecLite, CODEC>::value,
                "CODEC should be derived from ProtobufCodecLite");

 public:
  using ConcreteMessagePtr = std::shared_ptr<MSG>;
  using ProtobufMessageCallback = std::function<void(
      const TcpConnectionPtr&, const ConcreteMessagePtr&, TimeStamp)>;
  using RawMessageCallback = ProtobufCodecLite::RawMessageCallback;
  using ErrorCallback = ProtobufCodecLite::ErrorCallback;

  explicit ProtobufCodecLiteT(ProtobufMessageCallback messageCallback,
                              const RawMessageCallback& rawMessageCallback = RawMessageCallback(),
                              const ErrorCallback& errorCallback = ProtobufCodecLite::defaultErrorCallback)
                              : messageCallback_(std::move(messageCallback)),
                                codec_(&MSG::default_instance(),
                                       TAG,
                                       std::bind(&ProtobufCodecLiteT::onRpcMessage, this, _1, _2, _3),
                                       rawMessageCallback,
                                       errorCallback)
  {}

  const std::string& tag() const {
    return codec_.tag();
  }

  void send(const TcpConnectionPtr& conn, const MSG& message) {
    codec_.send(conn, message);
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp receiveTime) {
    codec_.onMessage(conn, buf, receiveTime);
  }

  void fillEmptyBuffer(Libel::net::Buffer* buffer, const MSG& message) {
    codec_.fillEmptyBuffer(buffer, message);
  }

private:
  void onRpcMessage(const TcpConnectionPtr& conn, const MessagePtr& message, TimeStamp receiveTime) {
    messageCallback_(conn, ::Libel::down_pointer_cast<MSG>(message), receiveTime);
  }

private:
  ProtobufMessageCallback messageCallback_;
  CODEC codec_;
};

}  // namespace net
}  // namespace Libel

#endif  // LIBEL_PROTOBUFCODECLITE_H
