//
// Created by kaymind on 2020/12/11.
//

#include "libel/net/channel.h"
#include "libel/base/logging.h"
#include "libel/net/buffer.h"
#include "libel/net/eventloop.h"

#include <functional>
#include <map>

#include <sys/timerfd.h>
#include <unistd.h>
#include <cstdio>

using namespace Libel;
using namespace Libel::net;

void print(const char* msg) {
  static std::map<const char*, TimeStamp> lasts;
  TimeStamp& last = lasts[msg];
  TimeStamp now = TimeStamp::now();
  printf("%s tid %d %s delay %f\n", now.toString().c_str(),
         CurrentThread::tid(), msg, timeDiffInSeconds(now, last));
  last = now;
}

namespace Libel {

namespace net {

namespace detail {

/// two functions defined in @file timerqueue.cpp
int createTimerfd();
void readTimerfd(int timerfd, TimeStamp now);

}  // namespace detail
}  // namespace net
}  // namespace Libel

class PeriodicTimer {
 public:
  PeriodicTimer(EventLoop* loop, double intervalSeconds, TimerCallback cb)
      : loop_(loop),
        timerfd_(Libel::net::detail::createTimerfd()),
        timerfdChannel_(loop_, timerfd_),
        intervalSeconds_(intervalSeconds),
        cb_(std::move(cb)) {
    timerfdChannel_.setReadCallback(
        std::bind(&PeriodicTimer::handleRead, this));
    timerfdChannel_.enableReading();
  }

  void start() {
    struct itimerspec spec {};
    memZero(&spec, sizeof(spec));
    spec.it_interval = toTimeSpec(intervalSeconds_);
    spec.it_value = spec.it_interval;
    int ret =
        ::timerfd_settime(timerfd_, 0 /* relative timer */, &spec, nullptr);
    if (ret) LOG_ERROR << "timerfd_settime() error:" << strerror(errno);
  }

  ~PeriodicTimer() {
    timerfdChannel_.disableAll();
    timerfdChannel_.removeSelfFromLoop();
    ::close(timerfd_);
  }

 private:
  void handleRead() {
    loop_->assertInLoopThread();
    Libel::net::detail::readTimerfd(timerfd_, TimeStamp::now());
    if (cb_) cb_();
  }

  static struct timespec toTimeSpec(double seconds) {
    struct timespec ts {};
    memZero(&ts, sizeof(ts));
    const int64_t kNanoSecondsPerSecond = 1e9, kMinInterval = 1e5;
    auto ns = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
    ns = std::max(kMinInterval, ns);
    ts.tv_nsec = static_cast<long>(ns % kNanoSecondsPerSecond);
    ts.tv_sec = static_cast<time_t>(ns / kNanoSecondsPerSecond);
    return ts;
  }

 private:
  EventLoop* loop_;
  const int timerfd_;
  Channel timerfdChannel_;
  const double intervalSeconds_;
  TimerCallback cb_;
};

int main() {
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid()
           << " Try adjusting the wall clock, see what happens.";
  EventLoop loop;
  PeriodicTimer timer(&loop, 1, std::bind(print, "PeriodicTimer"));
  timer.start();
  loop.runEvery(1, std::bind(print, "Eventloop::runEvery"));
  loop.loop();
  return 0;
}