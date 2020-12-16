//
// Created by kaymind on 2020/12/15.
//

#include "libel/net/eventloop_thread.h"
#include "libel/net/eventloop.h"
#include "libel/base/Thread.h"
#include "libel/base/countdown_latch.h"

#include <cstdio>
#include <unistd.h>

using namespace Libel;
using namespace Libel::net;

void print(EventLoop* loop = nullptr) {
  printf("print: pid = %d, tid = %d, loop = %p\n", getpid(), CurrentThread::tid(), loop);
}

void quit(EventLoop *loop) {
  print(loop);
  loop->quit();
}

int main() {
  print();

  {
    EventLoopThread thr1; // never start
  }

  {
    // dtor calls quit()
    EventLoopThread thr2;
    auto loop = thr2.startLoop();
    assert(loop != nullptr);
    loop->runInLoop(std::bind(print, loop));
    CurrentThread::sleepUsec(500*1000);
  }
  {
    // quit() before dtor
    EventLoopThread thr3;
    auto loop = thr3.startLoop();
    assert(loop != nullptr);
    loop->runInLoop(std::bind(quit, loop));
    CurrentThread::sleepUsec(500*1000);
  }
}
