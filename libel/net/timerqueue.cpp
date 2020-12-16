//
// Created by kaymind on 2020/12/14.
//

#include "libel/net/timerqueue.h"

#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/timer.h"
#include "libel/net/timerId.h"

#include <sys/timerfd.h>
#include <unistd.h>

namespace Libel {

namespace net {

namespace detail {

int createTimerfd() {
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
  if (timerfd < 0) {
    LOG_FATAL << "failed to call timerfd_create";
  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(TimeStamp when) {
  int64_t ms =
      when.microSecondsSinceEpoch() - TimeStamp::now().microSecondsSinceEpoch();
  if (ms < 100) ms = 100;
  struct timespec ts {};
  ts.tv_sec = static_cast<time_t>(ms / TimeStamp::kMicroSecondsPerSecond);
  ts.tv_nsec =
      static_cast<long>((ms % TimeStamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

void readTimerfd(int timerfd, TimeStamp now) {
  uint64_t num;
  ssize_t n = ::read(timerfd, &num, sizeof(num));
  LOG_TRACE << "TimerQueue::handleRead() " << num << " at " << now.toString();
  if (n != sizeof(num)) {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n
              << " bytes instead of 8";
  }
}

void resetTimerfd(int timerfd, TimeStamp expiration) {
  /// wake up loop by timerfd_settime()
  struct itimerspec newValue {};
  struct itimerspec oldValue {};
  memZero(&newValue, sizeof(newValue));
  memZero(&oldValue, sizeof(oldValue));
  newValue.it_value = howMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret) {
    LOG_ERROR << "failed to call timerfd_settime error:" << strerror(errno);
  }
}

}  // namespace detail
}  // namespace net
}  // namespace Libel

using namespace Libel;
using namespace Libel::net;
using namespace Libel::net::detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_(),
      callingExpiredTimers_(false) {
  timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
  timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
  timerfdChannel_.disableAll();
  timerfdChannel_.removeSelfFromLoop();
  ::close(timerfd_);
}

TimerId TimerQueue::addTimer(TimerCallback cb, TimeStamp when, double interval) {
  std::shared_ptr<Timer> timer(new Timer(std::move(cb), when, interval));
  loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
  return TimerId{timer, timer->sequence()};
}

void TimerQueue::cancel(const TimerId &timerId) {
  loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(const std::shared_ptr<Timer> &timer) {
  loop_->assertInLoopThread();
  bool earliestChanged = insert(timer);
  if (earliestChanged) {
    resetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::cancelInLoop(const TimerId &timerId) {
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  ActiveTimer timer(timerId.timer_, timerId.sequence_);
  auto iter = activeTimers_.find(timer);
  if (iter != activeTimers_.end()) {
    size_t n = timers_.erase(Entry(iter->first->expiration(), iter->first));
    assert(n == 1); (void)n;
    activeTimers_.erase(iter);
  } else if (callingExpiredTimers_) {
    cancelingTimers_.insert(timer);
  }
  assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead() {
  loop_->assertInLoopThread();
  TimeStamp now(TimeStamp::now());
  readTimerfd(timerfd_, now);
  std::vector<Entry> expired = getExpired(now);
  callingExpiredTimers_ = true;
  cancelingTimers_.clear();

  for (const auto& it : expired)
    it.second->run();
  callingExpiredTimers_ = false;
  reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Libel::TimeStamp now) {
  assert(timers_.size() == activeTimers_.size());
  std::vector<Entry> expired;
  std::shared_ptr<Timer> null(nullptr);
  TimeStamp nextMs(now.microSecondsSinceEpoch() + 1);
  Entry sentry(nextMs, null);
  /// notice that we use nullptr as the value of Entry's second parameter
  /// so we add now's microSecondsSinceEpoch 1 to avoid missing timeout timers
  auto end = timers_.lower_bound(sentry);
  assert(end == timers_.end() || now < end->first);
  std::copy(timers_.begin(), end, back_inserter(expired));
  timers_.erase(timers_.begin(), end);

  for (const auto& it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    size_t n = activeTimers_.erase(timer);
    assert(n == 1); (void)n;
  }
  assert(timers_.size() == activeTimers_.size());
  return expired;
}

void TimerQueue::reset(const std::vector<Entry> &expired, Libel::TimeStamp now) {
  TimeStamp nextExpire;

  for (const auto& it : expired) {
    ActiveTimer timer(it.second, it.second->sequence());
    if (it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
      it.second->restart(now);
      insert(it.second);
    }
  }
  if (!timers_.empty()) {
    nextExpire = timers_.begin()->second->expiration();
  }
  if (nextExpire.valid()) {
    resetTimerfd(timerfd_, nextExpire);
  }
}

bool TimerQueue::insert(const std::shared_ptr<Timer> &timer) {
  loop_->assertInLoopThread();
  bool earliestChanged = false;
  TimeStamp when = timer->expiration();
  auto it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliestChanged = true;
  }
  {
    auto result = timers_.insert(Entry(when, timer));
    assert(result.second); (void) result;
  }
  {
    auto result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second); (void) result;
  }
  assert(timers_.size() == activeTimers_.size());
  return earliestChanged;
}


















