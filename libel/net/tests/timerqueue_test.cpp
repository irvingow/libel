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
std::unique_ptr<TimerQueue> timerQueue = nullptr;

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
}

int main() {
  EventLoop loop;
  testCancelTimer(&loop);
  testAddTimer(&loop);
  loop.loop();
}