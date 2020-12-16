//
// Created by kaymind on 2020/11/25.
//

#ifndef LIBEL_TIMESTAMP_H
#define LIBEL_TIMESTAMP_H

#include <algorithm>
#include <string>

namespace Libel {

class TimeStamp {
 public:
  TimeStamp() : microSecondsSinceEpoch_(0) {}

  explicit TimeStamp(int64_t microSecondsSinceEpoch)
      : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

  void swap(TimeStamp &rhs) {
    std::swap(microSecondsSinceEpoch_, rhs.microSecondsSinceEpoch_);
  }

  std::string toString() const;
  std::string toFormattedString(bool showMicroSeconds = true) const;

  bool valid() const { return microSecondsSinceEpoch_ > 0; }

  int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

  time_t secondsSinceEpoch() const {
    return static_cast<time_t>(microSecondsSinceEpoch_ /
                               kMicroSecondsPerSecond);
  }

  static TimeStamp now();

  static TimeStamp invalid() {
      return {};
  }

  static TimeStamp fromUnixTime(time_t t) {
      return fromUnixTime(t, 0);
  }

  static TimeStamp fromUnixTime(time_t t, int microseconds) {
      return TimeStamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
  }

  static const int kMicroSecondsPerSecond = 1000 * 1000;

 private:
  int64_t microSecondsSinceEpoch_;
};

inline bool operator<(TimeStamp lhs, TimeStamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator>(TimeStamp lhs, TimeStamp rhs) {
    return lhs.microSecondsSinceEpoch() > rhs.microSecondsSinceEpoch();
}

inline bool operator==(TimeStamp lhs, TimeStamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline double timeDiffInSeconds(TimeStamp high, TimeStamp low) {
    auto diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / TimeStamp::kMicroSecondsPerSecond;
}

inline TimeStamp addTime(TimeStamp timeStamp, double seconds) {
    auto delta = static_cast<int64_t>(seconds * TimeStamp::kMicroSecondsPerSecond);
    return TimeStamp(timeStamp.microSecondsSinceEpoch() + delta);
}

}

#endif  // LIBEL_TIMESTAMP_H
