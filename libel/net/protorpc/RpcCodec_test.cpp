//
// Created by kaymind on 2021/1/10.
//

#undef NDEBUG
#include "libel/net/protorpc/RpcCodec.h"
#include "libel/net/buffer.h"
#include "libel/net/protobuf/ProtobufCodecLite.h"
#include "libel/net/protorpc/rpc.pb.h"

#include <cstdio>

using namespace Libel;
using namespace Libel::net;

void rpcMessageCallback(const TcpConnectionPtr&, const RpcMessagePtr&, TimeStamp) {
}

MessagePtr g_msgptr;

void messageCallback(const TcpConnectionPtr&,
                     const MessagePtr& msg,
                     TimeStamp) {
  g_msgptr = msg;
}

void print(const Buffer& buffer) {
  printf("encoded to %zd bytes\n", buffer.readableBytes());
  for (size_t i = 0; i < buffer.readableBytes(); ++i) {
    auto ch = static_cast<unsigned char>(buffer.peek()[i]);
    printf("%2zd: 0x%02x %c\n", i, ch, isgraph(ch) ? ch : ' ');
  }
}

char rpctag[] = "RPC0";

int main() {
  RpcMessage message;
  message.set_type(REQUEST);
  message.set_id(2);
  char wire[] = "\0\0\0\x13" "RPC0" "\x08\x01\x11\x02\0\0\0\0\0\0\0" "\x0f\xef\x01\x32";
  std::string expected(wire, sizeof(wire) - 1);
  std::string s1, s2;
  Buffer buffer1, buffer2;
  {
    RpcCodec codec(rpcMessageCallback);
    codec.fillEmptyBuffer(&buffer1, message);
    print(buffer1);
    s1 = buffer1.toString();
  }
  {
    ProtobufCodecLite codec(&RpcMessage::default_instance(), "RPC0", messageCallback);
    codec.fillEmptyBuffer(&buffer2, message);
    print(buffer2);
    s2 = buffer2.toString();
    codec.onMessage(TcpConnectionPtr(), &buffer1, TimeStamp::now());
    assert(g_msgptr);
    assert(g_msgptr->DebugString() == message.DebugString());
    g_msgptr.reset();
  }
  assert(s1 == s2);
  assert(s1 == expected);

  {
    Buffer buffer;
    ProtobufCodecLite codec(&RpcMessage::default_instance(), "XYZ", messageCallback);
    codec.fillEmptyBuffer(&buffer, message);
    print(buffer);
    codec.onMessage(TcpConnectionPtr(), &buffer, TimeStamp::now());
    assert(g_msgptr);
    assert(g_msgptr->DebugString() == message.DebugString());
  }
  google::protobuf::ShutdownProtobufLibrary();
}




















