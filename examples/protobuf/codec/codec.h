//
// Created by kaymind on 2021/1/13.
//

#ifndef LIBEL_CODEC_H
#define LIBEL_CODEC_H

#include "libel/net/buffer.h"
#include "libel/net/tcp_connection.h"

#include <google/protobuf/message.h>

// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   int32_t len;
//   int32_t nameLen;
//   char    typeName[nameLen];
//   char    protobufData[len - 4 - nameLen - 4];
//   int32_t checkSum; // adler32 of nameLen, typeName and protobufData
// }

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

class ProtobufCodec : Libel::noncopyable {
 public:
  enum ErrorCode {
    kNoError = 0,
    kInvalidLength,
    kCheckSumError,
    kInvalidNameLen,
    kUnknownMessageType,
    kParseError,
  };

  using ProtobufMessageCallback =
      std::function<void(const Libel::net::TcpConnectionPtr&, const MessagePtr&,
                         Libel::TimeStamp)>;

  using ErrorCallback =
      std::function<void(const Libel::net::TcpConnectionPtr&,
                         Libel::net::Buffer*, Libel::TimeStamp, ErrorCode)>;

  explicit ProtobufCodec(ProtobufMessageCallback messageCallback)
      : messageCallback_(std::move(messageCallback)),
        errorCallback_(defaultErrorCallback) {}

  ProtobufCodec(ProtobufMessageCallback messageCallback,
                ErrorCallback errorCallback)
      : messageCallback_(std::move(messageCallback)),
        errorCallback_(std::move(errorCallback)) {}

  void onMessage(const Libel::net::TcpConnectionPtr& conn,
                 Libel::net::Buffer* buffer, Libel::TimeStamp receiveTime);

  void send(const Libel::net::TcpConnectionPtr& conn,
            const google::protobuf::Message& message) {
    Libel::net::Buffer buffer;
  }

  static const std::string& errorCodeToString(ErrorCode errorCode);
  static void fillEmptyBuffer(Libel::net::Buffer* buffer,
                              const google::protobuf::Message& message);
  static google::protobuf::Message* createMessage(const std::string& type_name);
  static MessagePtr parse(const char* buffer, int len, ErrorCode* errorCode);

 private:
  static void defaultErrorCallback(const Libel::net::TcpConnectionPtr&,
                                   Libel::net::Buffer*, Libel::TimeStamp,
                                   ErrorCode);

  ProtobufMessageCallback messageCallback_;
  ErrorCallback errorCallback_;

  const static uint32_t kHeaderLen = sizeof(int32_t);
  const static uint32_t kMinMessageLen =
      2 * kHeaderLen + 2;  // nameLen + typeName + checkSum
  const static uint32_t kMaxMessageLen = 64 * 1024 * 1024;
};

#endif  // LIBEL_CODEC_H
