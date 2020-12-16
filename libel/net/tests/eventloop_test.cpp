//
// Created by kaymind on 2020/12/14.
//

#include "libel/net/eventloop.h"
#include "libel/base/Thread.h"

#include <cassert>
#include <cstdio>
#include <unistd.h>

using namespace Libel;
using namespace Libel::net;

EventLoop* g_loop = nullptr;

void callback() {
  printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  /// program will abort here because there are two eventloop in one thread.
  EventLoop anotherLoop;
}

void threadFunc(void *) {
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  assert(EventLoop::getEventLoopOfCurrentThead() == nullptr);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThead() == &loop);
  loop.runAfter(1.0, callback);
  loop.loop();
}

int main() {
  printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  assert(EventLoop::getEventLoopOfCurrentThead() == nullptr);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThead() == &loop);

  Thread thread(threadFunc, nullptr);
  thread.start();
  loop.loop();
}