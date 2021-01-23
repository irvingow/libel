//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/codec/dispatcher.h"

#include "examples/protobuf/codec/query.pb.h"

#include <iostream>

using std::cout;
using std::endl;

using QueryPtr = std::shared_ptr<Libel::Query>;
using AnswerPtr = std::shared_ptr<Libel::Answer>;

void test_down_pointer_case() {
  std::shared_ptr<google::protobuf::Message> msg(new Libel::Query);
  std::shared_ptr<Libel::Query> query(
      Libel::down_pointer_cast<Libel::Query>(msg));
  assert(msg && query);
  if (!query) {
    abort();
  }
}

void onQuery(const Libel::net::TcpConnectionPtr&, const QueryPtr& message,
             Libel::TimeStamp) {
  cout << "OnQuery: " << message->GetTypeName() << endl;
}

// users get cooret message type when their callbacks are called.
void onAnswer(const Libel::net::TcpConnectionPtr&, const AnswerPtr& message,
              Libel::TimeStamp) {
  cout << "OnAnswer: " << message->GetTypeName() << endl;
}

void onUnknownMessageType(const Libel::net::TcpConnectionPtr&,
                          const MessagePtr& message, Libel::TimeStamp) {
  cout << "OnUnknownMessageType: " << message->GetTypeName() << endl;
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  test_down_pointer_case();

  ProtobufDispatcher dispatcher(onUnknownMessageType);
  dispatcher.registerMessageCallback<Libel::Query>(onQuery);
  dispatcher.registerMessageCallback<Libel::Answer>(onAnswer);

  Libel::net::TcpConnectionPtr conn;
  Libel::TimeStamp timeStamp;

  std::shared_ptr<Libel::Query> query(new Libel::Query);
  std::shared_ptr<Libel::Answer> answer(new Libel::Answer);
  std::shared_ptr<Libel::Empty> empty(new Libel::Empty);
  dispatcher.onProtobufMessage(conn, query, timeStamp);
  dispatcher.onProtobufMessage(conn, answer, timeStamp);
  dispatcher.onProtobufMessage(conn, empty, timeStamp);

  google::protobuf::ShutdownProtobufLibrary();
}
