//
// Created by kaymind on 2020/11/24.
//

#include <string>
#include <stdio.h>
#include <unistd.h>

#include "libel/base/Thread.h"
#include "libel/base/current_thread.h"

void mysleep(int seconds) {
    timespec t = {seconds, 0};
    nanosleep(&t, nullptr);
}

void threadFunc(void *) {
    printf("tid = %d\n", Libel::CurrentThread::tid());
}

void threadFunc2(int x, void *) {
    printf("tid = %d, x = %d\n", Libel::CurrentThread::tid(), x);
}

void threadFunc3(void *name) {
    auto str = reinterpret_cast<std::string *>(name);
    printf("tid = %d, name = %s\n", Libel::CurrentThread::tid(), str == nullptr ? "anonymous" : str->c_str());
    mysleep(1);
}

class Foo {
public:
    explicit Foo(double x) : x_(x) {}

    void memberFunc(void *name) {
        auto str = reinterpret_cast<std::string *>(name);
        printf("tid = %d, Foo::x_ = %f, name = %s\n", Libel::CurrentThread::tid(), x_, str->c_str());
    }

    void memberFunc2(const std::string& text, void *) {
        printf("tid = %d, Foo::x_ = %f, text = %s\n", Libel::CurrentThread::tid(), x_, text.c_str());
    }

private:
    double x_;

};

int main() {
    printf("pid = %d, tid = %d\n", ::getpid(), Libel::CurrentThread::tid());

    Libel::Thread t1(threadFunc, nullptr);
    t1.start();
    printf("t1.tid = %d\n", t1.tid());
    t1.join();

    Libel::Thread t2(std::bind(threadFunc2, 42, std::placeholders::_1), nullptr, "thread for free function with argument");
    t2.start();
    printf("t2.tid = %d\n", t2.tid());
    t2.join();

    Foo foo(12.23);
    std::string name = "Mr liu";
    Libel::Thread t3(std::bind(&Foo::memberFunc, &foo, std::placeholders::_1), &name, "thread for member function with pointer");
    t3.start();
    t3.join();

    Libel::Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo), "test", std::placeholders::_1), nullptr);
    t4.start();
    t4.join();

    {
        Libel::Thread t5(threadFunc3, &name);
        t5.start();
        // t5 thread might destruct earlier than thread creation
    }
    mysleep(2);
    {
        Libel::Thread t6(threadFunc3, nullptr);
        t6.start();
        // ensure that t6 destruct later than thread creation
        mysleep(2);
    }
    sleep(2);
    printf("number of created threads %d\n", Libel::Thread::getCurCount());
}






