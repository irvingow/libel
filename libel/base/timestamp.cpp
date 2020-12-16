//
// Created by kaymind on 2020/11/25.
//

#include "libel/base/timestamp.h"

#include <sys/time.h>
#include <cinttypes>
#include <cstdio>

using namespace Libel;

static_assert(sizeof(TimeStamp) == sizeof(int64_t),
              "TimeStamp should be same size as int64_t");

std::string TimeStamp::toString() const {
  char buf[32] = {0};
  int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
  int64_t ms = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, ms);
  return buf;
}

std::string TimeStamp::toFormattedString(bool showMicroSeconds) const {
  char buf[64] = {0};
  auto seconds =
      static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  struct tm time {};
  gmtime_r(&seconds, &time);

  if (showMicroSeconds) {
    auto ms =
        static_cast<int>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             time.tm_year + 1900, time.tm_mon + 1, time.tm_mday, time.tm_hour,
             time.tm_min, time.tm_sec, ms);
  } else {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
             time.tm_hour, time.tm_min, time.tm_sec);
  }
  return buf;
}

TimeStamp TimeStamp::now() {
  struct timeval tv {};
  gettimeofday(&tv, nullptr);
  int64_t seconds = tv.tv_sec;
  return TimeStamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}