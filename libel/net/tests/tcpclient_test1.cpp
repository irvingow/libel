//
// Created by kaymind on 2020/12/18.
//

#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/tcp_client.h"

using namespace Libel;
using namespace Libel::net;

TcpClient* g_client;

void timeout() {
  LOG_INFO << "timeout";
  g_client->stop();
}

int main(int argc, char *argv[]) {
  EventLoop loop;
  InetAddress serverAddr("127.0.0.1", 2); // no such server
  TcpClient client(&loop, serverAddr, "TcpClient");
  g_client = &client;
  loop.runAfter(0.0, timeout);
  loop.runAfter(1.0, std::bind(&EventLoop::quit, &loop));
  client.connect();
  CurrentThread::sleepUsec(100*1000);
  loop.loop();
}
