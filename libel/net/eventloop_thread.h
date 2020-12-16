//
// Created by kaymind on 2020/12/3.
//

#ifndef LIBEL_EVENTLOOP_THREAD_H
#define LIBEL_EVENTLOOP_THREAD_H

#include "libel/base/condition.h"
#include "libel/base/Mutex.h"
#include "libel/base/Thread.h"

namespace Libel {

namespace net {

class EventLoop;

class EventLoopThread : noncopyable {
public:
    using ThreadInitCallback = std::function<void (EventLoop*)>;

    EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), const std::string &name = "");
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();

    EventLoop *loop_ GUARDED_BY(mutex_);
    MutexLock mutex_;
    Condition cond_ GUARDED_BY(mutex_);
    bool exiting_;
    Thread thread_;
    ThreadInitCallback callback_;
};

}

}

#endif //LIBEL_EVENTLOOP_THREAD_H
