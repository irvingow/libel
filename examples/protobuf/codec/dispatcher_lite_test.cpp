//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/codec/dispatcher_lite.h"

#include "examples/protobuf/codec/query.pb.h"

#include <iostream>

using std::cout;
using std::endl;

void onUnknownMessageType(const Libel::net::TcpConnectionPtr&,
                          const MessagePtr& message,
                          Libel::TimeStamp) {
  cout << "onUnknownMessageType: " << message->GetTypeName() << endl;
}

// users have to do down type cast by themselves
void onQuery(const Libel::net::TcpConnectionPtr&,
             const MessagePtr& message,
             Libel::TimeStamp) {
  cout << "onQuery: " << message->GetTypeName() << endl;
  auto query = Libel::down_pointer_cast<Libel::Query>(message);
  assert(query != nullptr);
}

void onAnswer(const Libel::net::TcpConnectionPtr&,
             const MessagePtr& message,
             Libel::TimeStamp) {
  cout << "onAnswer: " << message->GetTypeName() << endl;
  auto answer = Libel::down_pointer_cast<Libel::Answer>(message);
  assert(answer != nullptr);
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ProtobufDispatcherLite dispatcherLite(onUnknownMessageType);
  dispatcherLite.registerMessageCallback(Libel::Query::descriptor(), onQuery);
  dispatcherLite.registerMessageCallback(Libel::Answer::descriptor(), onAnswer);

  Libel::net::TcpConnectionPtr conn;
  Libel::TimeStamp timeStamp;

  std::shared_ptr<Libel::Query> query(new Libel::Query);
  std::shared_ptr<Libel::Answer> answer(new Libel::Answer);
  std::shared_ptr<Libel::Empty> empty(new Libel::Empty);
  dispatcherLite.onProtobufMessage(conn, query, timeStamp);
  dispatcherLite.onProtobufMessage(conn, answer, timeStamp);
  dispatcherLite.onProtobufMessage(conn, empty, timeStamp);

  google::protobuf::ShutdownProtobufLibrary();
}