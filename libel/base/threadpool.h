//
// Created by kaymind on 2020/11/23.
//

#ifndef LIBEL_THREADPOOL_H
#define LIBEL_THREADPOOL_H

#include "libel/base/condition.h"
#include "libel/base/Mutex.h"
#include "libel/base/Thread.h"

#include <deque>
#include <vector>

namespace Libel {

class ThreadPool : noncopyable {
public:
    typedef std::function<void ()> Task;

    explicit ThreadPool(std::string nameArg = std::string("ThreadPool"));
    ~ThreadPool();

    void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize;}

    void setThreadInitCallback(const Task &cb) { threadInitCallback_ = cb; }

    void start(int numThreads);

    void stop();

    const std::string &name() const { return name_; }

    size_t queueSize() const;

    void run(Task f);

private:
    bool isFull() const REQUIRES(mutex_);
    void runInThread();
    Task take();

    mutable MutexLock mutex_;
    Condition notEmpty_ GUARDED_BY(mutex_);
    Condition notFull_ GUARDED_BY(mutex_);
    std::string name_;
    Task threadInitCallback_;
    std::vector<std::unique_ptr<Libel::Thread>> threads_;
    std::deque<Task> queue_ GUARDED_BY(mutex_);
    size_t maxQueueSize_;
    bool running_;
};

}

#endif //LIBEL_THREADPOOL_H
