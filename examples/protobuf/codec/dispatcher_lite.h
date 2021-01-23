//
// Created by kaymind on 2021/1/13.
//

#ifndef LIBEL_DISPATCHER_LITE_H
#define LIBEL_DISPATCHER_LITE_H

#include "libel/base/noncopyable.h"
#include "libel/net/callbacks.h"

#include <google/protobuf/message.h>

#include <map>

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

class ProtobufDispatcherLite : Libel::noncopyable {
 public:
  using ProtobufMessageCallback =
      std::function<void(const Libel::net::TcpConnectionPtr&, const MessagePtr&,
                         Libel::TimeStamp)>;

  explicit ProtobufDispatcherLite(ProtobufMessageCallback defaultCb)
      : defaultCallback_(std::move(defaultCb)) {}

  void onProtobufMessage(const Libel::net::TcpConnectionPtr& conn,
                         const MessagePtr& message,
                         Libel::TimeStamp receiveTime) const {
    auto iter = callbacks_.find(message->GetDescriptor());
    if (iter != callbacks_.end()) {
      iter->second(conn, message, receiveTime);
    } else {
      defaultCallback_(conn, message, receiveTime);
    }
  }

  void registerMessageCallback(const google::protobuf::Descriptor* descriptor,
                               const ProtobufMessageCallback& callback) {
    callbacks_[descriptor] = callback;
  }

 private:
  using CallbackMap =
      std::map<const google::protobuf::Descriptor*, ProtobufMessageCallback>;
  CallbackMap callbacks_;
  ProtobufMessageCallback defaultCallback_;
};

#endif  // LIBEL_DISPATCHER_LITE_H
