//
// Created by kaymind on 2020/11/25.
//

#include "libel/base/countdown_latch.h"
#include "libel/base/Mutex.h"

#include <unistd.h>

using namespace Libel;

int g_count = 0;
CountDownLatch g_countDownLatch1(1);
CountDownLatch g_countDownLatch2(3);
MutexLock g_mutex;

void threadFunc(void *) {
    g_countDownLatch1.wait();
    g_countDownLatch2.countDown();
    MutexLockGuard lock(g_mutex);
    ++g_count;
}

int main() {
    g_count = 0;
    Libel::Thread t1(threadFunc, nullptr);
    Libel::Thread t2(threadFunc, nullptr);
    Libel::Thread t3(threadFunc, nullptr);
    t1.start();
    t2.start();
    t3.start();
    usleep(100);
    assert(g_count == 0);
    g_countDownLatch1.countDown();
    t1.join();
    t2.join();
    t3.join();
    assert(g_count == 3);
    return 0;
}
