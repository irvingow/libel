//
// Created by kaymind on 2020/12/14.
//

#ifndef LIBEL_TIMER_H
#define LIBEL_TIMER_H

#include <atomic>

#include "libel/base/noncopyable.h"
#include "libel/base/timestamp.h"
#include "libel/net/callbacks.h"

namespace Libel {

namespace net {

///
/// Internal class for timer event.
///
class Timer : noncopyable {
 public:
  Timer(TimerCallback cb, TimeStamp when, double interval)
      : callback_(std::move(cb)),
        expiration_(when),
        interval_(interval),
        repeat_(interval_ > 0.0),
        sequence_(s_numCreated_++) {}

  void run() const { callback_(); }

  TimeStamp expiration() const { return expiration_; }

  bool repeat() const { return repeat_; }

  int64_t sequence() const { return sequence_; }

  void restart(TimeStamp now);

  static int64_t numCreated() { return s_numCreated_; }

 private:
  const TimerCallback callback_;
  TimeStamp expiration_;
  const double interval_;
  const bool repeat_;
  const int64_t sequence_;

  static std::atomic<int64_t> s_numCreated_;
};

}  // namespace net
}  // namespace Libel

#endif  // LIBEL_TIMER_H
