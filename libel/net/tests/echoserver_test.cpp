//
// Created by kaymind on 2020/12/17.
//

#include "libel/base/Thread.h"
#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/inet_address.h"
#include "libel/net/tcp_server.h"

#include <unistd.h>

using namespace Libel;
using namespace Libel::net;

int numThreads = 0;

class EchoServer {
 public:
  EchoServer(EventLoop* loop, const InetAddress& listenAddr)
      : loop_(loop), server_(loop, listenAddr, "EchoServer") {
    server_.setConnectionCallback(
        std::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&EchoServer::onMessage, this, _1, _2, _3));
    server_.setThreadNum(numThreads);
  }

  void start() { server_.start(); }

 private:
  void onConnection(const TcpConnectionPtr& connectionPtr) {
    LOG_TRACE << connectionPtr->peerAddress().toIpPort() << " -> "
              << connectionPtr->localAddress().toIpPort() << " is "
              << (connectionPtr->connected() ? "UP" : "DOWN");
    LOG_INFO << connectionPtr->getTcpInfoString();
  }
  void onMessage(const TcpConnectionPtr& connectionPtr, Buffer* buffer,
                 TimeStamp time) {
    std::string msg(buffer->retrieveAllAsString());
    LOG_INFO << connectionPtr->name() << " recv " << msg.size() << " bytes at "
              << time.toString();
    if (msg == "quit\n")
      loop_->quit();
    connectionPtr->send(msg);
  }

 private:
  EventLoop* loop_;
  TcpServer server_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " <<getpid()<<", tid = " << CurrentThread::tid();
  LOG_INFO << "sizeof TcpConnection = " << sizeof(TcpConnection);
  if (argc > 1)
    numThreads = atoi(argv[1]);
  bool ipv6 = argc > 2;
  EventLoop loop;
  InetAddress listenAddr(2000, false, ipv6);
  EchoServer server(&loop, listenAddr);

  server.start();
  loop.loop();
}