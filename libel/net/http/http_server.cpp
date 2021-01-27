//
// Created by kaymind on 2020/12/20.
//

#include "libel/net/http/http_server.h"

#include "libel/base/logging.h"
#include "libel/net/http/http_context.h"
#include "libel/net/http/http_request.h"
#include "libel/net/http/http_response.h"

using namespace Libel;
using namespace Libel::net;

namespace Libel {

namespace net {

namespace detail {

void defaultHttpCallback(const HttpRequest &, HttpResponse *resp) {
  resp->setStatusCode(HttpResponse::k404NotFound);
  resp->setStatusMessage("Not found");
  resp->setCloseConnection(true);
}

}  // namespace detail
}  // namespace net
}  // namespace Libel

HttpServer::HttpServer(EventLoop *loop, const InetAddress &listenAddr,
                       const std::string &name, TcpServer::Option option)
    : server_(loop, listenAddr, name, option),
      httpCallback_(detail::defaultHttpCallback) {
  server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, _1));
  server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

void HttpServer::start() {
  LOG_WARN << "HttpServer[" << server_.name()
  << "] starts listening on " << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &connection) {
  if (connection->connected()) {
    std::shared_ptr<void> context = std::make_shared<HttpContext>();
    connection->setContext(context);
  }
}

void HttpServer::onMessage(const TcpConnectionPtr &connection, Buffer *buffer, TimeStamp receiveTime) {
  LOG_DEBUG << "receive request:" << buffer->toString();
  std::shared_ptr<HttpContext> context = std::static_pointer_cast<HttpContext>(connection->getContext());
  if (!context->parseRequest(buffer, receiveTime)) {
    connection->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    connection->shutdown();
  }
  if (context->gotAll()) {
    onRequest(connection, context->getRequest());
    context->reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr &conn, const HttpRequest& req) {
  const std::string &connection = req.getHeader("Connection");
  bool close = connection == "close" || (req.getVersion() == HttpRequest::kHttp10 && connection != "KeepAlive");
  HttpResponse response(close);
  httpCallback_(req, &response);
  Buffer buffer;
  response.appendToBuffer(&buffer);
  conn->send(&buffer);
  if (response.closeConnection())
    conn->shutdown();
}