//
// Created by kaymind on 2020/11/23.
//

#include "libel/base/threadpool.h"

#include <cassert>
#include <exception>

using namespace Libel;

ThreadPool::ThreadPool(std::string nameArg)
    : mutex_(), notEmpty_(mutex_), notFull_(mutex_), name_(std::move(nameArg)), maxQueueSize_(0), running_(false) {}


ThreadPool::~ThreadPool() {
    if (running_)
        stop();
}

void ThreadPool::start(int numThreads) {
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        char id[32];
        snprintf(id, sizeof(id), "%d", i+1);
        threads_.emplace_back(new Libel::Thread(std::bind(&ThreadPool::runInThread, this), nullptr, name_+id));
        threads_[i]->start();
    }
    if (numThreads == 0 && threadInitCallback_)
        threadInitCallback_();
}

void ThreadPool::stop() {
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        notEmpty_.notifyAll();
        notFull_.notifyAll();
    }
    for (auto& thr : threads_) {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const {
    MutexLockGuard lock(mutex_);
    return queue_.size();
}

void ThreadPool::run(Task task) {
    if (threads_.empty())
        task(); // if not threads in threadPool, run task in this thread
    else {
        MutexLockGuard lock(mutex_);
        while (isFull() && running_) {
            notFull_.wait();
        }
        if (!running_) return;
        assert(!isFull());
        queue_.push_back(std::move(task));
        notEmpty_.notify();
    }
}

ThreadPool::Task ThreadPool::take() {
    MutexLockGuard lock(mutex_);
    // to avoid spurious wakeup, so use while loop
    while (queue_.empty() && running_) {
        notEmpty_.wait();
    }
    Task task;
    if (!queue_.empty()) {
        task = queue_.front();
        queue_.pop_front();
        if (maxQueueSize_ > 0)
            notFull_.notify();
    }
    return task;
}

bool ThreadPool::isFull() const {
    mutex_.assertLocked();
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread() {
    try {
        if (threadInitCallback_)
            threadInitCallback_();
        while (running_) {
            Task task(take());
            if (task)
                task();
        }
    } catch (const std::exception& ex) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason %s\n", ex.what());
    } catch (...) {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw;
    }

}










