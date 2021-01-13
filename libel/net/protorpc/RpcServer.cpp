//
// Created by kaymind on 2021/1/10.
//

#include "libel/net/protorpc/RpcServer.h"

#include "libel/base/logging.h"
#include "libel/net/protorpc/RpcChannel.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

using namespace Libel;
using namespace Libel::net;

RpcServer::RpcServer(EventLoop *loop, const InetAddress &listenAddr)
: server_(loop, listenAddr, "RpcServer") {
  server_.setConnectionCallback(std::bind(&RpcServer::onConnection, this, _1));
}

void RpcServer::registerService(::google::protobuf::Service *service) {
  auto desc = service->GetDescriptor();
  services_[desc->full_name()] = service;
}

void RpcServer::start() {
  server_.start();
}

void RpcServer::onConnection(const TcpConnectionPtr &connection) {
  LOG_INFO << "RpcServer - " << connection->peerAddress().toIpPort() << " -> "
  << connection->localAddress().toIpPort() << " is "
  << (connection->connected() ? "UP" : "DOWN");
  if (connection->connected()) {
    RpcChannelPtr channelPtr(new RpcChannel(connection));
    channelPtr->setServices(&services_);
    connection->setMessageCallback(std::bind(&RpcChannel::onMessage, get_pointer(channelPtr), _1, _2, _3));
    connection->setContext(channelPtr);
  } else {
    connection->setContext(RpcChannelPtr());
  }
}

