//
// Created by kaymind on 2021/1/10.
//

#include "libel/net/protorpc/RpcChannel.h"

#include "libel/base/logging.h"
#include "libel/net/protorpc/rpc.pb.h"

#include <google/protobuf/descriptor.h>

using namespace Libel;
using namespace Libel::net;

RpcChannel::RpcChannel()
    : codec_(std::bind(&RpcChannel::onRpcMessage, this, _1, _2, _3)),
      id_(0),
      services_(nullptr) {
  LOG_INFO << "RpcChannel::ctor -" << this;
}

RpcChannel::RpcChannel(TcpConnectionPtr conn)
    : codec_(std::bind(&RpcChannel::onRpcMessage, this, _1, _2, _3)),
      conn_(std::move(conn)),
      id_(),
      services_(nullptr) {}

RpcChannel::~RpcChannel() {
  LOG_INFO << "RpcChannel::dtor - " << this;
  for (const auto &outstanding : outstandings_) {
    auto out = outstanding.second;
    delete out.response;
    delete out.done;
  }
}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor *method,
                            ::google::protobuf::RpcController *controller,
                            const ::google::protobuf::Message *request,
                            ::google::protobuf::Message *response,
                            ::google::protobuf::Closure *done) {
  RpcMessage message;
  message.set_type(REQUEST);
  message.set_id(++id_);
  message.set_service(method->service()->full_name());
  LOG_DEBUG << "service full_name is:" << method->service()->full_name();
  message.set_method(method->name());
  LOG_DEBUG << "method is:" << method->name();
  message.set_request(request->SerializeAsString());

  // register info to outstandings_
  OutstandingCall out = {response, done};
  {
    MutexLockGuard lock(mutexLock_);
    outstandings_[id_] = out;
  }
  // send request to server
  codec_.send(conn_, message);
}

void RpcChannel::onMessage(const TcpConnectionPtr &conn, Buffer *buffer,
                           TimeStamp receiveTime) {
  codec_.onMessage(conn, buffer, receiveTime);
}

void RpcChannel::onRpcMessage(const TcpConnectionPtr &conn,
                              const RpcMessagePtr &messagePtr,
                              TimeStamp receiveTime) {
  assert(conn == conn_);
  RpcMessage &message = *messagePtr;
  if (message.type() == MessageType::RESPONSE) {
    // called when client receive RESPONSE from server
    auto id = message.id();
    assert(message.has_response() || message.has_error());
    OutstandingCall out = {nullptr, nullptr};
    {
      MutexLockGuard lock(mutexLock_);
      auto iter = outstandings_.find(id);
      if (iter != outstandings_.end()) {
        out = iter->second;
        outstandings_.erase(iter);
      }
    }
    if (out.response) {
      // out.response is newed in user file,
      // out.response is deleted after d exit scope
      std::unique_ptr<google::protobuf::Message> d(out.response);
      if (message.has_response()) {
        out.response->ParseFromString(message.response());
      }
      if (out.done) {
        out.done->Run();
      }
    }
  } else if (message.type() == REQUEST) {
    // called when server receive REQUEST from client
    ErrorCode errorCode = WRONG_PROTO;
    if (services_) {
      auto iter = services_->find(message.service());
      if (iter != services_->end()) {
        // notice that here message and service come from two .proto files
        // for example, service means sudoku service
        // while message means message defined in rpc.proto
        google::protobuf::Service *service = iter->second;
        assert(service != nullptr);
        const google::protobuf::ServiceDescriptor *descriptor =
            service->GetDescriptor();
        // we get methodName from message.method()
        // and we get related methodDesciptor from descriptor->FindMethodByName()
        const google::protobuf::MethodDescriptor *methodDescriptor =
            descriptor->FindMethodByName(message.method());
        if (methodDescriptor) {
          // we get request object from methodDescriptor and service->GetRequestPrototype()
          // for example, if methodDescriptor is the related MethodDescriptor of
          // method "sudoku.Solve", request is the type of "SudokuRequest"
          std::unique_ptr<google::protobuf::Message> request(
              service->GetRequestPrototype(methodDescriptor).New());
          if (request->ParseFromString(message.request())) {
            // similar to request, we get response object from
            // methodDesciptor and service->GetResponsePrototype()
            // for example, if methodDescriptor is the related MethodDescriptor of
            // method "sudoku.Solve", response is the type of "SudokuResponse"
            google::protobuf::Message *response =
                service->GetResponsePrototype(methodDescriptor).New();
            // response is deleted in doneCallback
            auto id = message.id();
            service->CallMethod(
                methodDescriptor, nullptr, get_pointer(request), response,
                google::protobuf::NewCallback(this, &RpcChannel::doneCallback,
                                              response, id));
            errorCode = NO_ERROR;
          } else {
            errorCode = INVALID_REQUEST;
          }
        } else {
          errorCode = NO_METHOD;
        }
      }
    } else {
      errorCode = NO_SERVICE;
    }
    if (errorCode != NO_ERROR) {
      RpcMessage response;
      response.set_type(RESPONSE);
      response.set_id(message.id());
      response.set_error(errorCode);
      codec_.send(conn_, response);
    }
  } else if (message.type() == ERROR) {
    LOG_WARN << "message type is ERROR";
  }
}

void RpcChannel::doneCallback(::google::protobuf::Message *response,
                              uint64_t id) {
  std::unique_ptr<google::protobuf::Message> d(response);
  RpcMessage message;
  message.set_type(RESPONSE);
  message.set_id(id);
  message.set_response(response->SerializeAsString());
  codec_.send(conn_, message);
}
