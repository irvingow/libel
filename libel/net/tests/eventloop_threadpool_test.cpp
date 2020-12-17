//
// Created by kaymind on 2020/12/17.
//

#include "libel/net/eventloop_threadpool.h"
#include "libel/net/eventloop.h"
#include "libel/base/Thread.h"

#include <atomic>
#include <cstdio>
#include <unistd.h>

using namespace Libel;
using namespace Libel::net;

std::atomic_int initCount{0};

void print(EventLoop* loop = nullptr) {
  printf("main(): pid = %d, tid = %d, loop = %p\n", getpid(), CurrentThread::tid(), loop);
}

void init(EventLoop *loop) {
  initCount++;
  printf("init(): pid = %d, tid = %d, loop = %p\n", getpid(), CurrentThread::tid(), loop);
}

int main() {
  print();
  EventLoop loop;
  loop.runAfter(11, std::bind(&EventLoop::quit, &loop));

  initCount = 0;
  {
    printf("Single thread %p\n", &loop);
    EventLoopThreadPool eventLoopThreadPool(&loop, "single");
    eventLoopThreadPool.setThreadNum(0);
    eventLoopThreadPool.start(init);
    assert(eventLoopThreadPool.getNextLoop() == &loop);
    assert(eventLoopThreadPool.getNextLoop() == &loop);
    assert(eventLoopThreadPool.getNextLoop() == &loop);
  }
  assert(initCount == 1);

  initCount = 0;
  {
    printf("Another thread:\n");
    EventLoopThreadPool eventLoopThreadPool(&loop, "another");
    eventLoopThreadPool.setThreadNum(1);
    eventLoopThreadPool.start(init);
    auto nextLoop = eventLoopThreadPool.getNextLoop();
    nextLoop->runAfter(2, std::bind(print, nextLoop));
    assert(nextLoop != &loop);
    assert(nextLoop == eventLoopThreadPool.getNextLoop());
    assert(nextLoop == eventLoopThreadPool.getNextLoop());
    ::sleep(3);
  }
  assert(initCount == 1);

  initCount = 0;
  {
    printf("Three threads:\n");
    EventLoopThreadPool eventLoopThreadPool(&loop, "three");
    eventLoopThreadPool.setThreadNum(3);
    eventLoopThreadPool.start(init);
    auto nextLoop = eventLoopThreadPool.getNextLoop();
    nextLoop->runInLoop(std::bind(print, nextLoop));
    assert(nextLoop != &loop);
    assert(nextLoop != eventLoopThreadPool.getNextLoop());
    assert(nextLoop != eventLoopThreadPool.getNextLoop());
    assert(nextLoop == eventLoopThreadPool.getNextLoop());
  }
  assert(initCount == 3);
  loop.loop();
}
