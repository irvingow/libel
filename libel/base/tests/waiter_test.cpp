#include "libel/base/waiter.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "libel/base/Thread.h"
#include "libel/base/current_thread.h"

using namespace Libel;

void func1(void* obj) {
  std::cout << "thread id:"
            << std::string(CurrentThread::tidString(),
                           CurrentThread::getTidStrLength())
            << std::endl;
  std::cout << "thread name:" << CurrentThread::threadName << std::endl;
  Waiter* waiter = static_cast<Waiter*>(obj);
  std::cout << "func1 finished" << std::endl;
  waiter->finish();
}

void func2(void* obj) {
  std::cout << "thread id:"
            << std::string(CurrentThread::tidString(),
                           CurrentThread::getTidStrLength())
            << std::endl;
  std::cout << "thread name:" << CurrentThread::threadName << std::endl;
  Waiter* waiter = static_cast<Waiter*>(obj);
  std::cout << "func2 finished" << std::endl;
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  waiter->finish();
}

void func3(void* obj) {
  std::cout << "thread id:"
            << std::string(CurrentThread::tidString(),
                           CurrentThread::getTidStrLength())
            << std::endl;
  std::cout << "thread name:" << CurrentThread::threadName << std::endl;
  Waiter* waiter = static_cast<Waiter*>(obj);
  waiter->wait();
  std::cout << "func3 finished" << std::endl;
}

int main() {
  Waiter waiter(2);
  Thread thread1(func1, &waiter, "thread1");
  Thread thread2(func2, &waiter, "thread2");
  {
    Thread thread3(func3, &waiter, "thread3");
    thread3.start();
  }
  thread1.start();
  thread2.start();
  thread1.join();
  thread2.join();
  return 0;
}