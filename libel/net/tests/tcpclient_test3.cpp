//
// Created by kaymind on 2020/12/18.
//

#include "libel/base/logging.h"
#include "libel/net/eventloop_thread.h"
#include "libel/net/tcp_client.h"
#include "libel/base/current_thread.h"

using namespace Libel;
using namespace Libel::net;

int main() {
  Logger::setLogLevel(Logger::DEBUG);

  EventLoopThread loopThread;
  {
    InetAddress serverAddr("127.0.0.1", 1234); // normal server
    TcpClient client(loopThread.startLoop(), serverAddr, "TcpClient");
    client.connect();
    CurrentThread::sleepUsec(500*1000); // wait for connect
    client.disconnect();
  }
  CurrentThread::sleepUsec(1000*1000);
}