//
// Created by kaymind on 2020/12/14.
//

#include "libel/net/timer.h"

using namespace Libel;
using namespace Libel::net;

std::atomic<int64_t> Timer::s_numCreated_;

void Timer::restart(TimeStamp now) {
  if (repeat_) {
    expiration_ = addTime(now, interval_);
  } else {
    expiration_ = TimeStamp::invalid();
  }
}
