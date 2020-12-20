//
// Created by kaymind on 2020/12/20.
//

#ifndef LIBEL_HTTP_SERVER_H
#define LIBEL_HTTP_SERVER_H

#include "libel/net/tcp_server.h"

namespace Libel {

namespace net {

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It's not a fully HTTP 1.1 compliant server, but provides minimum features,
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class HttpServer : noncopyable {
 public:
  using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

  HttpServer(EventLoop* loop, const InetAddress& listenAddr,
             const std::string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  EventLoop* getLoop() const { return server_.getLoop(); }

  /// not thread safe, callback be registered before calling start().
  void setHttpCallback(HttpCallback cb) {
    httpCallback_ = std::move(cb);
  }

  void setThreadNum(int numThreads) {
    server_.setThreadNum(numThreads);
  }

  void start();

 private:
  void onConnection(const TcpConnectionPtr& connection);
  void onMessage(const TcpConnectionPtr& connection, Buffer* buffer,
                 TimeStamp receiveTime);
  void onRequest(const TcpConnectionPtr&, const HttpRequest&);

  TcpServer server_;
  HttpCallback httpCallback_;
};

}  // namespace net
}  // namespace Libel

#endif  // LIBEL_HTTP_SERVER_H
