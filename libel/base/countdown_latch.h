//
// Created by kaymind on 2020/11/23.
//

#ifndef LIBEL_COUNTDOWN_LATCH_H
#define LIBEL_COUNTDOWN_LATCH_H

#include "libel/base/condition.h"
#include "libel/base/Mutex.h"

namespace Libel {

class CountDownLatch : noncopyable {
public:
    explicit CountDownLatch(int count);

    void wait();

    void countDown();

    int getCount() const;

private:
    mutable MutexLock mutex_;
    Condition condition_ GUARDED_BY(mutex_);
    int count_ GUARDED_BY(mutex_);
};

}

#endif //LIBEL_COUNTDOWN_LATCH_H
