//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/codec/codec.h"
#include "examples/protobuf/codec/dispatcher.h"
#include "examples/protobuf/codec/query.pb.h"

#include "libel/base/Mutex.h"
#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/tcp_server.h"

#include <unistd.h>
#include <cstdio>

using namespace Libel;
using namespace Libel::net;

using QueryPtr = std::shared_ptr<Libel::Query>;
using AnswerPtr = std::shared_ptr<Libel::Answer>;

class QueryServer : noncopyable {
 public:
  QueryServer(EventLoop* loop, const InetAddress& listenAddr)
      : server_(loop, listenAddr, "QueryServer"),
        dispatcher_(
            std::bind(&QueryServer::onUnknownMessage, this, _1, _2, _3)),
        codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_,
                         _1, _2, _3)) {
    dispatcher_.registerMessageCallback<Libel::Query>(
        std::bind(&QueryServer::onQuery, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<Libel::Answer>(
        std::bind(&QueryServer::onAnswer, this, _1, _2, _3));
    server_.setConnectionCallback(
        std::bind(&QueryServer::onConnection, this, _1));
    server_.setMessageCallback(
        std::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
  }

  void start() { server_.start(); }

 private:
  void onConnection(const TcpConnectionPtr& connection) {
    LOG_INFO << connection->peerAddress().toIpPort() << " -> "
             << connection->localAddress().toIpPort() << " is "
             << (connection->connected() ? "UP" : "DOWN");
  }

  void onUnknownMessage(const TcpConnectionPtr& conn, const MessagePtr& message,
                        TimeStamp) {
    LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
    conn->shutdown();
  }

  void onQuery(const Libel::net::TcpConnectionPtr& conn, const QueryPtr& query,
               Libel::TimeStamp) {
    LOG_INFO << "onQuery:\n" << query->GetTypeName() << query->DebugString();
    Answer answer;
    answer.set_id(1);
    answer.set_questioner("Liu wj");
    answer.set_answerer("irvinglwj.github.io");
    answer.add_solution("Jump!");
    codec_.send(conn, answer);

    conn->shutdown();
  }

  void onAnswer(const Libel::net::TcpConnectionPtr& conn,
                const AnswerPtr& answer, Libel::TimeStamp) {
    LOG_INFO << "onAnswer: " << answer->GetTypeName();
    conn->shutdown();
  }

 private:
  TcpServer server_;
  ProtobufDispatcher dispatcher_;
  ProtobufCodec codec_;
};

int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getpid();
  if (argc > 1) {
    EventLoop loop;
    auto port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress serverAddr(port);
    QueryServer server(&loop, serverAddr);
    server.start();
    loop.loop();
  }

  return 0; }
