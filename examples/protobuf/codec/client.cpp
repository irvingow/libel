//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/codec/codec.h"
#include "examples/protobuf/codec/dispatcher.h"
#include "examples/protobuf/codec/query.pb.h"

#include "libel/base/Mutex.h"
#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/tcp_client.h"

#include <unistd.h>
#include <cstdio>

using namespace Libel;
using namespace Libel::net;

using EmptyPtr = std::shared_ptr<Libel::Empty>;
using AnswerPtr = std::shared_ptr<Libel::Answer>;

google::protobuf::Message* messageToSend;

class QueryClient : noncopyable {
 public:
  QueryClient(EventLoop* loop, const InetAddress& serverAddr)
      : loop_(loop),
        client_(loop, serverAddr, "QueryClient"),
        dispatcher_(
            std::bind(&QueryClient::onUnknownMessage, this, _1, _2, _3)),
        codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_,
                         _1, _2, _3)) {
    dispatcher_.registerMessageCallback<Libel::Answer>(
        std::bind(&QueryClient::onAnswer, this, _1, _2, _3));
    dispatcher_.registerMessageCallback<Libel::Empty>(
        std::bind(&QueryClient::onEmpty, this, _1, _2, _3));
    client_.setConnectionCallback(
        std::bind(&QueryClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&ProtobufCodec::onMessage, &codec_, _1, _2, _3));
  }

  void connect() { client_.connect(); }

 private:
  void onConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected()) {
      codec_.send(conn, *messageToSend);
    } else {
      loop_->quit();
    }
  }
  void onUnknownMessage(const TcpConnectionPtr&, const MessagePtr& message,
                        TimeStamp) {
    LOG_INFO << "onUnKnownMessage: " << message->GetTypeName();
  }

  void onAnswer(const Libel::net::TcpConnectionPtr&, const AnswerPtr& answer,
                Libel::TimeStamp) {
    LOG_INFO << "onAnswer:\n" << answer->GetTypeName() << answer->DebugString();
  }

  void onEmpty(const Libel::net::TcpConnectionPtr&, const EmptyPtr& answer,
               Libel::TimeStamp) {
    LOG_INFO << "onEmpty: " << answer->GetTypeName();
  }

 private:
  EventLoop* loop_;
  TcpClient client_;
  ProtobufDispatcher dispatcher_;
  ProtobufCodec codec_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << getpid();
  if (argc > 2) {
    EventLoop loop;
    auto port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress serverAddr(argv[1], port);

    Libel::Query query;
    query.set_id(1);
    query.set_questioner("Liu wj");
    query.add_question("Running?");

    Libel::Empty empty;
    messageToSend = &query;

    if (argc > 3 && argv[3][0] == 'e') messageToSend = &empty;

    QueryClient client(&loop, serverAddr);
    client.connect();
    loop.loop();
  } else {
    printf("Usage: %s host_ip port [q|e]\n", argv[0]);
  }
  return 0;
}
