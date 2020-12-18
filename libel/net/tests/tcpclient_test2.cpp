//
// Created by kaymind on 2020/12/18.
//

#include "libel/net/tcp_client.h"
#include "libel/base/Thread.h"
#include "libel/net/eventloop.h"
#include "libel/base/logging.h"

using namespace Libel;
using namespace Libel::net;

void threadFunc(void *data) {
  auto loop = reinterpret_cast<EventLoop*>(data);
  InetAddress serverAddr("127.0.0.1", 1234); // normal server
  TcpClient client(loop, serverAddr, "TcpClient");
  client.connect();

  CurrentThread::sleepUsec(1000*1000);
  /// client destructs when connected.
}

int main() {
  Logger::setLogLevel(Logger::DEBUG);

  EventLoop loop;
  loop.runAfter(3.0, std::bind(&EventLoop::quit, &loop));
  Thread thr(threadFunc, &loop);
  thr.start();
  loop.loop();
}