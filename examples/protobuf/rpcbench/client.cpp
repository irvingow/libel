//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/rpcbench/echo.pb.h"

#include "libel/base/countdown_latch.h"
#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/eventloop_threadpool.h"
#include "libel/net/inet_address.h"
#include "libel/net/protorpc/RpcChannel.h"
#include "libel/net/tcp_client.h"
#include "libel/net/tcp_connection.h"

#include <unistd.h>
#include <cstdio>

using namespace Libel;
using namespace Libel::net;

static const int kRequests = 50000;

class RpcClient : noncopyable {
 public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr,
            CountDownLatch* allConnected, CountDownLatch* allFinished)
      : client_(loop, serverAddr, "RpcClient"),
        channel_(new RpcChannel),
        stub_(get_pointer(channel_)),
        allConnected_(allConnected),
        allFinished_(allFinished),
        count_(0) {
    client_.setConnectionCallback(
        std::bind(&RpcClient::onConnection, this, _1));
    client_.setMessageCallback(
        std::bind(&RpcChannel::onMessage, get_pointer(channel_), _1, _2, _3));
  }

  void connect() { client_.connect(); }
  void sendRequest() {
    echo::EchoRequest request;
    request.set_payload("001010");
    echo::EchoResponse* response = new echo::EchoResponse;
    stub_.Echo(
        nullptr, &request, response,
        google::protobuf::NewCallback(this, &RpcClient::replied, response));
  }

 private:
  void onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
      conn->setTcpNoDelay(true);
      channel_->setConnection(conn);
      allConnected_->countDown();
    }
  }

  void replied(echo::EchoResponse* response) {
    ++count_;
    if (count_ < kRequests) {
      sendRequest();
    } else {
      LOG_INFO << "last request response:" << response->payload();
      LOG_INFO << "RpcClient " << this << " finshed";
      allFinished_->countDown();
    }
  }

  TcpClient client_;
  RpcChannelPtr channel_;
  echo::EchoService::Stub stub_;
  CountDownLatch* allConnected_;
  CountDownLatch* allFinished_;
  int count_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << getpid();
  if (argc > 1) {
    int nClients = 1;
    if (argc > 2) {
      nClients = atoi(argv[2]);
    }
    int nThreads = 1;
    if (argc > 3) {
      nThreads = atoi(argv[3]);
    }
    CountDownLatch allConnected(nClients);
    CountDownLatch allFinished(nClients);

    EventLoop loop;
    EventLoopThreadPool pool(&loop, "rpcbench-client");
    pool.setThreadNum(nThreads);
    pool.start();
    InetAddress serverAddr(argv[1], 8888);

    std::vector<std::unique_ptr<RpcClient>> clients;
    for (int i = 0; i < nClients; ++i) {
      clients.emplace_back(new RpcClient(pool.getNextLoop(), serverAddr,
                                         &allConnected, &allFinished));
      clients.back()->connect();
    }
    allConnected.wait();
    TimeStamp start(TimeStamp::now());
    LOG_INFO << "all connected";
    for (int i = 0; i < nClients; ++i) {
      clients[i]->sendRequest();
    }
    allFinished.wait();
    TimeStamp end(TimeStamp::now());
    LOG_INFO << "all finished";
    double seconds = timeDiffInSeconds(end, start);
    printf("%f seconds\n", seconds);
    printf("%.1f calls per seconds\n", nClients * kRequests / seconds);

    exit(0);
  } else {
    printf("Usage: %s host_ip numClients [numThreads]\n", argv[0]);
  }
}
