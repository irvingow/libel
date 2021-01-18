//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/rpc/sudoku.pb.h"

#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/inet_address.h"
#include "libel/net/protorpc/RpcChannel.h"
#include "libel/net/tcp_client.h"
#include "libel/net/tcp_connection.h"

#include <unistd.h>
#include <cstdio>

using namespace Libel;
using namespace Libel::net;

class RpcClient : noncopyable {
 public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr)
      : loop_(loop),
        client_(loop, serverAddr, "RpcClient"),
        channel_(new RpcChannel),
        stub_(get_pointer(channel_)) {
    client_.setConnectionCallback(
        std::bind(&RpcClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage, get_pointer(channel_), _1, _2, _3));
  }

  void connect() { client_.connect(); }

 private:
  void onConnection(const TcpConnectionPtr& connection) {
    if (connection->connected()) {
      channel_->setConnection(connection);
      sudoku::SudokuRequest request;
      request.set_checkboard("001010");
      auto response = new sudoku::SudokuResponse;

      stub_.Solve(
          nullptr, &request, response,
          google::protobuf::NewCallback(this, &RpcClient::solved, response));
    } else {
      loop_->quit();
    }
  }

  void solved(sudoku::SudokuResponse* response) {
    LOG_INFO << "solved:\n" << response->DebugString();
    client_.disconnect();
  }

  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  sudoku::SudokuService::Stub stub_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << getpid();
  if (argc > 1) {
    EventLoop loop;
    InetAddress serverAddr(argv[1], 9981);

    RpcClient rpcClient(&loop, serverAddr);
    rpcClient.connect();
    loop.loop();
  } else {
    printf("Usage: %s host_ip\n", argv[0]);
  }
  google::protobuf::ShutdownProtobufLibrary();
}
