//
// Created by kaymind on 2020/12/14.
//

#ifndef LIBEL_TIMERQUEUE_H
#define LIBEL_TIMERQUEUE_H

#include "libel/base/Mutex.h"
#include "libel/base/timestamp.h"
#include "libel/net/callbacks.h"
#include "libel/net/channel.h"

#include <set>
#include <vector>

namespace Libel {

namespace net {

class EventLoop;
class Timer;
class TimerId;

///
/// No guarantee that the callback will be right on time
///
class TimerQueue : noncopyable {
public:
  explicit TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  TimerId addTimer(TimerCallback cb, TimeStamp when, double interval);

  void cancel(const TimerId&);

private:
  using Entry =std::pair<TimeStamp, std::shared_ptr<Timer>>;
  using TimerList = std::set<Entry>;
  using ActiveTimer = std::pair<std::shared_ptr<Timer>, int64_t>;
  using ActiveTimerSet = std::set<ActiveTimer>;

public:
  /// below functions are used for tests only
  TimerList getTimers() const { return timers_; }

  ActiveTimerSet getActiveTimers() const { return activeTimers_; }

private:
  void addTimerInLoop(const std::shared_ptr<Timer> &timer);
  void cancelInLoop(const TimerId &timerId);
  /// called when timerfd alarms
  void handleRead();
  /// move out all expired timers
  std::vector<Entry> getExpired(TimeStamp now);
  void reset(const std::vector<Entry>& expired, TimeStamp now);

  bool insert(const std::shared_ptr<Timer> &timer);

  EventLoop* loop_;
  const int timerfd_;
  Channel timerfdChannel_;
  /// Timer list sorted by expiration
  TimerList timers_;

  /// for cancel()
  ActiveTimerSet activeTimers_;
  std::atomic<bool> callingExpiredTimers_;
  ActiveTimerSet cancelingTimers_;
};


}
}

#endif //LIBEL_TIMERQUEUE_H
