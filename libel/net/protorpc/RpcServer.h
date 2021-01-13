//
// Created by kaymind on 2021/1/10.
//

#ifndef LIBEL_RPCSERVER_H
#define LIBEL_RPCSERVER_H

#include "libel/net/tcp_server.h"

namespace google {
namespace protobuf {

class Service;

}
}

namespace Libel {

namespace net {

class RpcServer {
public:
  RpcServer(EventLoop* loop, const InetAddress& listenAddr);

  void setThreadNum(int numThreads) {
    server_.setThreadNum(numThreads);
  }

  void registerService(::google::protobuf::Service*);

  void start();

private:
  void onConnection(const TcpConnectionPtr& connection);

  TcpServer server_;
  std::map<std::string, ::google::protobuf::Service*> services_;
};

}
}

#endif //LIBEL_RPCSERVER_H
