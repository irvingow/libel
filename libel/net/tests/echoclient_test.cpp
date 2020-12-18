//
// Created by kaymind on 2020/12/18.
//

#include "libel/base/Thread.h"
#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/inet_address.h"
#include "libel/net/tcp_client.h"

#include <unistd.h>
#include <cstdio>
#include <utility>

using namespace Libel;
using namespace Libel::net;

int numTheads = 0;
class EchoClient;
std::vector<std::unique_ptr<EchoClient>> clients;
int current = 0;

class EchoClient : noncopyable {
 public:
  EchoClient(EventLoop* loop, const InetAddress& serverAddr,
             const std::string& id)
      : loop_(loop) , client_(loop, serverAddr, "EchoClient" + id){
    client_.setConnectionCallback(std::bind(&EchoClient::onConnection, this, _1));
    client_.setMessageCallback(std::bind(&EchoClient::onMessage, this, _1, _2, _3));
  }

  void connect() { client_.connect(); }

 private:
  void onConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected()) {
      ++current;
      if (implicit_cast<size_t>(current) < clients.size())
        clients[static_cast<size_t>(current)]->connect();
      LOG_INFO << "*** connected " << current;
    }
    conn->send("world\n");
  }

  void onMessage(const TcpConnectionPtr& conn, Buffer* buffer,
                 TimeStamp timeStamp) {
    std::string msg(buffer->retrieveAllAsString());
    LOG_INFO << conn->name() << " recv " << msg.size() << " bytes at "
             << timeStamp.toString();
    if (msg == "quit\n") {
      conn->send("bye\n");
      conn->shutdown();
    } else if (msg == "shutdown")
      loop_->quit();
    else
      conn->send(msg);
  }

 private:
  EventLoop* loop_;
  TcpClient client_;
};


int main(int argc, char *argv[]) {
  LOG_INFO << "pid = " << getgid() << ", tid = " << CurrentThread::tid();
  if (argc > 1) {
    EventLoop loop;
    bool ipv6 = argc > 3;
    InetAddress serverAddr(argv[1], 2000, ipv6);

    int n = 1;
    if (argc > 2)
      n = atoi(argv[2]);
    clients.reserve(n);
    for (int i = 0; i < n; ++i) {
      char buf[32] = {};
      snprintf(buf, sizeof(buf), "%d", i+1);
      clients.emplace_back(new EchoClient(&loop, serverAddr, buf));
    }
    clients[current]->connect();
    loop.loop();
  } else {
    printf("Usage: %s host_ip [current#]\n", argv[0]);
  }
}
