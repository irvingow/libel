//
// Created by kaymind on 2020/11/24.
//

#include "libel/base/threadpool.h"
#include "libel/base/countdown_latch.h"
#include "libel/base/current_thread.h"
#include "libel/base/logging.h"

#include <unistd.h>
#include <cstdio>
#include <vector>
#include <sstream>

void print() { printf("tid = %d\n", Libel::CurrentThread::tid()); }

void printString(const std::string &str) {
  LOG_INFO << str;
  usleep(100 * 1000);
}

void test(int maxSize) {
  LOG_WARN << "Test ThreadPool with max queue size = " << maxSize;
  Libel::ThreadPool threadPool("MainThreadPool");
  threadPool.setMaxQueueSize(maxSize);
  threadPool.start(5);

  LOG_WARN << "Adding";
  threadPool.run(print);
  threadPool.run(print);
  std::vector<int> nums{1,2,3,4,5,6};
  Libel::CountDownLatch tmp(1);
  auto output = [&](std::stringstream& ss) {
    for (const auto& num : nums)
      ss << num;
    tmp.countDown();
  };
  std::stringstream ss;
  threadPool.run(std::bind(output, std::ref(ss)));
  tmp.wait();
  assert(ss.str() == "123456");
  for (int i = 0; i < 100; ++i) {
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "task %d", i);
    threadPool.run(std::bind(printString, std::string(buf)));
  }
  LOG_WARN << "Done";

  Libel::CountDownLatch latch(1);
  threadPool.run(std::bind(&Libel::CountDownLatch::countDown, &latch));
  latch.wait();
  threadPool.stop();
}

void longTask(int num) {
  LOG_INFO << "longTask" << num;
  usleep(3000000);
}

void test2() {
  LOG_WARN << "Test ThreadPool by stopping early";
  Libel::ThreadPool threadPool("ThreadPool");
  threadPool.setMaxQueueSize(5);
  threadPool.start(3);

  Libel::Thread thread1(
      [&threadPool](void *) {
        for (int i = 0; i < 20; ++i) {
          threadPool.run(std::bind(longTask, i));
        }
      },
      nullptr, "thread1");
  thread1.start();

  usleep(5000000);
  LOG_WARN << "stop pool";
  threadPool.stop(); // early stop

  thread1.join();
  // run() after stop ThreadPool
  threadPool.run(print);
  LOG_WARN << "test2 Done";
}

int main() {
    test(0);
    test(1);
    test(5);
    test(10);
    test(50);
    test2();
    return 0;
}


