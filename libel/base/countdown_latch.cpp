//
// Created by kaymind on 2020/11/23.
//

#include "libel/base/countdown_latch.h"

using namespace Libel;

CountDownLatch::CountDownLatch(int count) : mutex_(), condition_(mutex_), count_(count) {}

void CountDownLatch::wait() {
    MutexLockGuard lock(mutex_);
    while(count_ > 0) {
        condition_.wait();
    }
}

void CountDownLatch::countDown() {
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0)
        condition_.notifyAll();
}

int CountDownLatch::getCount() const {
    // really need lock?
    MutexLockGuard lock(mutex_);
    return count_;
}

