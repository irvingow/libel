//
// Created by kaymind on 2020/12/3.
//

#include "libel/net/eventloop_thread.h"
#include "libel/net/eventloop.h"

using namespace Libel;
using namespace Libel::net;

EventLoopThread::EventLoopThread(
    Libel::net::EventLoopThread::ThreadInitCallback cb, const std::string &name)
    : loop_(nullptr),
      mutex_(),
      cond_(mutex_),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), nullptr, name),
      callback_(std::move(cb)) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  // not thread safe
  if (loop_ != nullptr) {
    // if threadFunc exits just now, we call function with destructed object
    // but doesn't really matter because usually the program is going to exit
    loop_->quit();
    thread_.join();
  }
}

EventLoop * EventLoopThread::startLoop() {
    assert(!thread_.started());
    thread_.start();

    EventLoop* loop = nullptr;
    {
        MutexLockGuard lock(mutex_);
        // use while to avoid fake wake up
        while (loop_ == nullptr) {
            cond_.wait();
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;
    if (callback_)
        callback_(&loop);
    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();
    }
    loop.loop();
    MutexLockGuard lock(mutex_);
    loop_ = nullptr;
}