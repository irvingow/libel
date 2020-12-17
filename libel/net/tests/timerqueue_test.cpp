//
// Created by kaymind on 2020/12/15.
//

#include "libel/net/timerqueue.h"
#include "libel/base/timestamp.h"
#include "libel/net/eventloop.h"
#include "libel/net/timerId.h"

#include <memory>

using namespace Libel;
using namespace Libel::net;

size_t g_count = 0;
int cnt = 0;
EventLoop* g_loop;
std::unique_ptr<TimerQueue> timerQueue = nullptr;


void print(const char* msg) {
  printf("msg %s %s\n", TimeStamp::now().toString().c_str(), msg);
  if (++cnt == 20)
    g_loop->quit();
}

void cancel(const TimerId &timerId) {
  g_loop->cancel(timerId);
  printf("cancelled at %s\n", TimeStamp::now().toString().c_str());
}

void quit(EventLoop *loop) {
  loop->quit();
}

void timerCallback() {
  g_count--;
  assert(timerQueue->getActiveTimers().size() == g_count);
  assert(timerQueue->getTimers().size() == g_count);
}

void testAddTimer(EventLoop* loop) {
  g_count = 3;
  assert(timerQueue->getTimers().empty());
  assert(timerQueue->getActiveTimers().empty());
  auto timeStamp = addTime(TimeStamp::now(), 5);
  timerQueue->addTimer(timerCallback, timeStamp, 0);
  timeStamp = addTime(TimeStamp::now(), 10);
  timerQueue->addTimer(timerCallback, timeStamp, 0);
  timeStamp = addTime(TimeStamp::now(), 15);
  timerQueue->addTimer(timerCallback, timeStamp, 0);
  assert(timerQueue->getTimers().size() == 3);
  assert(timerQueue->getActiveTimers().size() == 3);
}

void testCancelTimer(EventLoop* loop) {
  g_count = 3;
  timerQueue.reset(new TimerQueue(loop));
  assert(timerQueue->getTimers().empty());
  assert(timerQueue->getActiveTimers().empty());
  auto timeStamp = addTime(TimeStamp::now(), 5);
  auto timerId5 = timerQueue->addTimer(timerCallback, timeStamp, 0);
  timeStamp = addTime(TimeStamp::now(), 10);
  auto timerId10 = timerQueue->addTimer(timerCallback, timeStamp, 0);
  timeStamp = addTime(TimeStamp::now(), 15);
  auto timerId15 = timerQueue->addTimer(timerCallback, timeStamp, 0);
  assert(timerQueue->getTimers().size() == 3);
  assert(timerQueue->getActiveTimers().size() == 3);
  timerQueue->cancel(timerId5);
  assert(timerQueue->getTimers().size() == 2);
  assert(timerQueue->getActiveTimers().size() == 2);
  timerQueue->cancel(timerId5);
  assert(timerQueue->getTimers().size() == 2);
  assert(timerQueue->getActiveTimers().size() == 2);
  timerQueue->cancel(timerId10);
  assert(timerQueue->getTimers().size() == 1);
  assert(timerQueue->getActiveTimers().size() == 1);
  timerQueue->cancel(timerId15);
  assert(timerQueue->getTimers().empty());
  assert(timerQueue->getActiveTimers().empty());
  loop->runAfter(3, std::bind(quit, loop));
}

int main() {
  {
    EventLoop loop;
    testCancelTimer(&loop);
    testAddTimer(&loop);
    loop.loop();
    /// we have to reset timerQueue because the corresponding eventloop is destroyed.
    timerQueue.reset();
  }
  {
    EventLoop loop;
    g_loop = &loop;
    loop.runAfter(1, std::bind(print, "once1"));
    loop.runAfter(1.5, std::bind(print, "once1.5"));
    loop.runAfter(2.5, std::bind(print, "once2.5"));
    loop.runAfter(3.5, std::bind(print, "once3.5"));
    TimerId timerId = loop.runAfter(4.5, std::bind(print, "once4.5"));
    loop.runAfter(4.2, std::bind(cancel, timerId));
    loop.runAfter(4.8, std::bind(cancel, timerId));
    loop.runEvery(2, std::bind(print, "every2"));
    auto timer3 = loop.runEvery(3, std::bind(print, "every3"));
    loop.runAfter(9.001, std::bind(cancel, timer3));

    loop.loop();
    print("main loop exists");
  }
}