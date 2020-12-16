//
// Created by kaymind on 2020/11/24.
//

#include "libel/base/Mutex.h"
#include "libel/base/Thread.h"
#include "libel/base/countdown_latch.h"

#include <cstdio>
#include <vector>

using namespace Libel;
using namespace std;

MutexLock g_mutex;
const int kCount = 10000;
int g_count = 0;

void threadFunc(void *) {
  for (int i = 0; i < kCount; ++i) {
    MutexLockGuard lock(g_mutex);
    ++g_count;
  }
}

int foo() {
  MutexLockGuard lock(g_mutex);
  if (!g_mutex.isLockedByThisThread()) {
    printf("Fail\n");
    return -1;
  }
  ++g_count;
  return 0;
}

int main() {
  MCHECK(foo());
  if (g_count != 1) {
    printf("MCHECK calls twice.\n");
    abort();
  }
  const int kMaxThreads = 10;
  for (int nthreads = 1; nthreads < kMaxThreads; ++nthreads) {
    std::vector<std::unique_ptr<Thread>> threads;
    g_count = 0;
    for (int i = 0; i < nthreads; ++i) {
      threads.emplace_back(new Thread(&threadFunc, nullptr));
      threads.back()->start();
    }
    for (int i = 0; i < nthreads; ++i) {
      threads[i]->join();
    }
    assert(g_count == nthreads * kCount);
  }
  return 0;
}
