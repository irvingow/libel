//
// Created by kaymind on 2021/1/13.
//

#ifndef LIBEL_DISPATCHER_H
#define LIBEL_DISPATCHER_H

#include "libel/base/noncopyable.h"
#include "libel/net/callbacks.h"

#include <google/protobuf/message.h>

#include <map>
#include <type_traits>

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

class Callback : Libel::noncopyable {
 public:
  virtual ~Callback() = default;
  virtual void onMessage(const Libel::net::TcpConnectionPtr&,
                         const MessagePtr& message, Libel::TimeStamp) const = 0;
};

template <typename T>
class CallbackT : public Callback {
  static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                "T must be derived from google::protobuf::Message");

 public:
  using ProtobufMessageTCallback =
      std::function<void(const Libel::net::TcpConnectionPtr&,
                         const std::shared_ptr<T>& message, Libel::TimeStamp)>;

  CallbackT(ProtobufMessageTCallback callback)
      : callback_(std::move(callback)) {}

  void onMessage(const Libel::net::TcpConnectionPtr& conn,
                 const MessagePtr& message,
                 Libel::TimeStamp receiveTime) const override {
    // using template and polymorphism to do down type cast
    std::shared_ptr<T> concrete = Libel::down_pointer_cast<T>(message);
    assert(concrete != nullptr);
    callback_(conn, concrete, receiveTime);
  }

 private:
  ProtobufMessageTCallback callback_;
};

class ProtobufDispatcher {
 public:
  using ProtobufMessageCallback =
      std::function<void(const Libel::net::TcpConnectionPtr&,
                         const MessagePtr& message, Libel::TimeStamp)>;

  explicit ProtobufDispatcher(ProtobufMessageCallback defaultCb)
      : defaultCallback_(std::move(defaultCb)) {}

  void onProtobufMessage(const Libel::net::TcpConnectionPtr& conn,
                         const MessagePtr& message,
                         Libel::TimeStamp receiveTime) const {
    auto iter = callbacks_.find(message->GetDescriptor());
    if (iter != callbacks_.end()) {
      iter->second->onMessage(conn, message, receiveTime);
    } else {
      defaultCallback_(conn, message, receiveTime);
    }
  }

  template <typename T>
  void registerMessageCallback(
      const typename CallbackT<T>::ProtobufMessageTCallback& callback) {
    std::shared_ptr<CallbackT<T>> pd(new CallbackT<T>(callback));
    callbacks_[T::descriptor()] = pd;
  }

 private:
  using CallbackMap =
      std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback>>;

  CallbackMap callbacks_;
  ProtobufMessageCallback defaultCallback_;
};

#endif  // LIBEL_DISPATCHER_H
