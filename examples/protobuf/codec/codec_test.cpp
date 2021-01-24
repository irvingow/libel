//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/codec/codec.h"
#include "examples/protobuf/codec/query.pb.h"
#include "libel/net/Endian.h"

#include <zlib.h>  // adler32
#include <cstdio>

using namespace Libel;
using namespace Libel::net;

void print(const Buffer& buffer) {
  printf("encoded to %zd bytes\n", buffer.readableBytes());
  for (size_t i = 0; i < buffer.readableBytes(); ++i) {
    unsigned char ch = static_cast<unsigned char>(buffer.peek()[i]);

    printf("%2zd: 0x%02x %c\n", i, ch, isgraph(ch) ? ch : ' ');
  }
}

void testQuery() {
  Libel::Query query;
  query.set_id(1);
  query.set_questioner("Liu wj");
  query.add_question("Running?");

  Buffer buffer;
  ProtobufCodec::fillEmptyBuffer(&buffer, query);
  print(buffer);

  const int32_t len = buffer.readInt32();
  assert(len == static_cast<int32_t>(buffer.readableBytes()));

  ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::parse(buffer.peek(), len, &errorCode);
  assert(errorCode == ProtobufCodec::kNoError);
  assert(message != nullptr);
  message->PrintDebugString();
  assert(message->DebugString() == query.DebugString());

  std::shared_ptr<Libel::Query> newQuery =
      down_pointer_cast<Libel::Query>(message);
  assert(newQuery != nullptr);
}

void testAnswer() {
  Libel::Answer answer;
  answer.set_id(1);
  answer.set_questioner("Liu wj");
  answer.set_answerer("irvinglwj.github.io");
  answer.add_solution("Jump!");
  answer.add_solution("Win!");

  Buffer buffer;
  ProtobufCodec::fillEmptyBuffer(&buffer, answer);
  print(buffer);

  const int32_t len = buffer.readInt32();
  assert(len == static_cast<int32_t>(buffer.readableBytes()));

  ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::parse(buffer.peek(), len, &errorCode);
  assert(errorCode == ProtobufCodec::kNoError);
  assert(message != nullptr);

  message->PrintDebugString();
  assert(message->DebugString() == answer.DebugString());

  std::shared_ptr<Libel::Answer> newAnswer =
      down_pointer_cast<Libel::Answer>(message);
  assert(newAnswer != nullptr);
}

void testEmpty() {
  Libel::Empty empty;

  Buffer buffer;
  ProtobufCodec::fillEmptyBuffer(&buffer, empty);
  print(buffer);

  const int32_t len = buffer.readInt32();
  assert(len == static_cast<int32_t>(buffer.readableBytes()));

  ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
  MessagePtr message = ProtobufCodec::parse(buffer.peek(), len, &errorCode);
  assert(message != nullptr);
  message->PrintDebugString();
  assert(message->DebugString() == empty.DebugString());
}

void redoCheckSum(std::string& data, int len) {
  int32_t checkSum =
      static_cast<int32_t>(sockets::hostToNetwork32(static_cast<uint32_t>(
          ::adler32(1, reinterpret_cast<const Bytef*>(data.c_str()),
                    static_cast<uint>(len - 4)))));
  data[len - 4] = reinterpret_cast<const char*>(&checkSum)[0];
  data[len - 3] = reinterpret_cast<const char*>(&checkSum)[1];
  data[len - 2] = reinterpret_cast<const char*>(&checkSum)[2];
  data[len - 1] = reinterpret_cast<const char*>(&checkSum)[3];
}

void testBadBuffer() {
  Libel::Empty empty;
  empty.set_id(43);

  Buffer buffer;
  ProtobufCodec::fillEmptyBuffer(&buffer, empty);

  const int32_t len = buffer.readInt32();
  assert(len == static_cast<int32_t>(buffer.readableBytes()));

  {
    std::string data(buffer.peek(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    // error: data len is not enough
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len - 1, &errorCode);
    assert(message == nullptr);
    assert(errorCode == ProtobufCodec::kCheckSumError);
  }

  {
    std::string data(buffer.peek(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[len-1]++;
    // error: data is corrupted
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kCheckSumError);
  }

  {
    std::string data(buffer.peek(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    // error: data is corrupted
    data[0]++;
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kCheckSumError);
  }

  {
    std::string data(buffer.peek(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[3] = 0;
    redoCheckSum(data, len);
    // error: data length is corrupted
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kInvalidNameLen);
  }

  {
    std::string data(buffer.peek(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[3] = 100;
    redoCheckSum(data, len);
    // error: data length is too long
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kInvalidNameLen);
  }

  {
    std::string data(buffer.peek(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[3]--;
    redoCheckSum(data, len);
    // error: typename is too short
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kUnknownMessageType);
  }

  {
    std::string data(buffer.peek(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[4] = 'M';
    redoCheckSum(data, len);
    // error: typename is wrong
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    assert(message == NULL);
    assert(errorCode == ProtobufCodec::kUnknownMessageType);
  }

  {
    std::string data(buffer.peek(), len);
    ProtobufCodec::ErrorCode errorCode = ProtobufCodec::kNoError;
    data[len - 6]--;
    redoCheckSum(data, len);
    MessagePtr message = ProtobufCodec::parse(data.c_str(), len, &errorCode);
    /// try to reproduce kParseError
//     assert(message == NULL);
//     assert(errorCode == ProtobufCodec::kParseError);
  }
}

int g_count = 0;

void onMessage(const Libel::net::TcpConnectionPtr& conn,
               const MessagePtr& message,
               Libel::TimeStamp ) {
  g_count++;
}

void testOnMessage() {
  Libel::Query query;
  query.set_id(1);
  query.set_questioner("Liu wj");
  query.add_question("Running?");

  Buffer buffer;
  ProtobufCodec::fillEmptyBuffer(&buffer, query);

  Libel::Empty empty;
  empty.set_id(43);
  empty.set_id(1982);

  Buffer buffer1;
  ProtobufCodec::fillEmptyBuffer(&buffer1, empty);

  size_t totalLen = buffer.readableBytes() + buffer1.readableBytes();
  Buffer all;
  all.append(buffer.peek(), buffer.readableBytes());
  all.append(buffer1.peek(), buffer1.readableBytes());
  assert(all.readableBytes() == totalLen);
  Libel::net::TcpConnectionPtr conn;
  Libel::TimeStamp timeStamp;
  ProtobufCodec codec(onMessage);
  for (size_t len = 0; len <= totalLen; ++len) {
    Buffer input;

    input.append(all.peek(), len);
    g_count = 0;
    codec.onMessage(conn, &input, timeStamp);
    int expected = len < buffer.readableBytes() ? 0 : 1;
    if (len == totalLen)
      expected = 2;
    assert(g_count == expected);
    (void) expected;
    input.append(all.peek()+ len, totalLen - len);
    codec.onMessage(conn, &input, timeStamp);
    assert(g_count == 2);
  }
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  testQuery();
  puts("");
  testAnswer();
  puts("");
  testEmpty();
  puts("");
  testBadBuffer();
  puts("");
  testOnMessage();
  puts("");

  puts("lucky!All pass!");
  google::protobuf::ShutdownProtobufLibrary();
}