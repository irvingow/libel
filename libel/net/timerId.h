//
// Created by kaymind on 2020/12/14.
//

#ifndef LIBEL_TIMERID_H
#define LIBEL_TIMERID_H

#include <memory>

namespace Libel {

namespace net {

class Timer;

///
/// An opaque identifier, for canceling Timer.
///
class TimerId {
public:
  TimerId() : timer_(nullptr), sequence_(0) {}

  TimerId(std::shared_ptr<Timer> timer, int64_t seq) : timer_(std::move(timer)), sequence_(seq) {}

  friend class TimerQueue;

private:
  std::shared_ptr<Timer> timer_;
  int64_t sequence_;
};


}
}

#endif //LIBEL_TIMERID_H
