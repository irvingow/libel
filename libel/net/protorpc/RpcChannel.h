//
// Created by kaymind on 2021/1/10.
//

#ifndef LIBEL_RPCCHANNEL_H
#define LIBEL_RPCCHANNEL_H

#include "libel/base/Mutex.h"
#include "libel/net/protorpc/RpcCodec.h"

#include <google/protobuf/service.h>

#include <map>

// Service and RpcChannel classes are incorporated from
// google/protobuf/service.h

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

namespace google {

namespace protobuf {

// Defined in other files
class Descriptor;         // descriptor.h
class ServiceDescriptor;  // descriptor.h
class MethodDescriptor;   // descritor.h
class Message;

class Closure;

class RpcController;
class Service;

}
}

namespace Libel {

namespace net {

// Abstract interface for an RPC channel. An RpcChannel represents a
// communication line to a Service which can be used to call that
// Service's methods. The service may be running on other machine.
// Normally, you should not call an RpcChannel directly, but instead
// construct a stub service wrapping it.
class RpcChannel : public ::google::protobuf::RpcChannel {
public:
  RpcChannel();

  explicit RpcChannel(TcpConnectionPtr conn);

  ~RpcChannel() override;

  void setConnection(const TcpConnectionPtr& conn) {
    conn_ = conn;
  }

  void setServices(const std::map<std::string, ::google::protobuf::Service*>* services) {
    services_ = services;
  }

  // Call the given method of the remote service. The signature of this
  // procedure looks the same as Service::CallMethod(), but the requirements
  // are less strict in one important way: the request and response objects
  // need not be of any specific class as long as their descriptor are
  // method->input_type() and method->output_type().
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  ::google::protobuf::RpcController* controller,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::google::protobuf::Closure* done) override;

  void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, TimeStamp receiveTime);

private:

  void onRpcMessage(const TcpConnectionPtr& conn,
                    const RpcMessagePtr& messagePtr,
                    TimeStamp receiveTime);

  void doneCallback(::google::protobuf::Message* response, uint64_t id);

  struct OutstandingCall {
    ::google::protobuf::Message* response;
    ::google::protobuf::Closure* done;
  };

  RpcCodec codec_;
  TcpConnectionPtr conn_;
  std::atomic<uint64_t> id_;

  MutexLock mutexLock_;
  std::map<uint64_t, OutstandingCall> outstandings_ GUARDED_BY(mutexLock_);

  const std::map<std::string, ::google::protobuf::Service*> *services_;
};

using RpcChannelPtr = std::shared_ptr<RpcChannel>;

}
}



















#endif //LIBEL_RPCCHANNEL_H
