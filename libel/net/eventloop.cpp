//
// Created by kaymind on 2020/11/25.
//

#include "libel/net/eventloop.h"
#include "libel/base/Mutex.h"
#include "libel/base/logging.h"
#include "libel/net/channel.h"
#include "libel/net/poller.h"
#include "libel/net/sockets_ops.h"
#include "libel/net/timerqueue.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <algorithm>

using namespace Libel;
using namespace Libel::net;

namespace {

__thread EventLoop* t_loopInThisThread = nullptr;

const int kPollTimeMs = 10000;

int createEventfd() {
  int efd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (efd < 0) {
    LOG_FATAL << "failed to call ::eventfd";
  }
  return efd;
}

}  // namespace

EventLoop* EventLoop::getEventLoopOfCurrentThead() {
  return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      iteration_(0),
      threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      context_(nullptr),
      currentActiveChannel_(nullptr) {
  LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;
  if (t_loopInThisThread) {
    LOG_FATAL << "another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  } else {
    t_loopInThisThread = this;
  }
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
  LOG_DEBUG << "Eventloop " << this << " of thread " << threadId_
            << " destructs in thread " << CurrentThread::tid();
  wakeupChannel_->disableAll();
  wakeupChannel_->removeSelfFromLoop();
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  if (quit_) {
    LOG_WARN << "quit() was called before loop()";
    return;
  }
  LOG_TRACE << "Eventloop " << this << " start looping";
  while (!quit_) {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    ++iteration_;
    if (Logger::getLogLevel() <= Logger::TRACE) {
      printActiveChannels();
    }
    eventHandling_ = true;
    for (Channel* channel : activeChannels_) {
      currentActiveChannel_ = channel;
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = nullptr;
    eventHandling_ = false;
    doPendingFunctors();
  }
  LOG_TRACE << "Eventloop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit() {
  quit_ = true;
  /// there is a chance that loop() just executes while(!quit_) and exits,
  /// then Eventloop destructs, then we are accessing an invalid object.

  //    if (!isInLoopThread()) {
  //        wakeup();
  //    }
}

void EventLoop::runInLoop(Functor cb) {
  if (isInLoopThread()) {
    cb();
  } else {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb) {
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.push_back(std::move(cb));
  }
  if (!isInLoopThread() || callingPendingFunctors_) wakeup();
}

size_t EventLoop::queueSize() const {
  MutexLockGuard lock(mutex_);
  return pendingFunctors_.size();
}

TimerId EventLoop::runAt(TimeStamp time, TimerCallback cb) {
  return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delaySeconds, TimerCallback cb) {
  TimeStamp time(addTime(TimeStamp::now(), delaySeconds));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double intervalSeconds, TimerCallback cb) {
  TimeStamp time(addTime(TimeStamp::now(), intervalSeconds));
  return timerQueue_->addTimer(std::move(cb), time, intervalSeconds);
}

void EventLoop::cancel(const TimerId& timerId) { timerQueue_->cancel(timerId); }

void EventLoop::updateChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_) {
    assert(currentActiveChannel_ == channel ||
           std::find(activeChannels_.begin(), activeChannels_.end(), channel) ==
               activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread() {
  LOG_FATAL << "Eventloop::abortNotInLoopThead - eventloop" << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = sockets::write(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one))
    LOG_ERROR << "Eventloop::wakeup() writes " << n << " bytes instead of 8";
}

void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = sockets::read(wakeupFd_, &one, sizeof(one));
  if (n != sizeof(one))
    LOG_ERROR << "Eventloop::handleRead() reads " << n << " bytes instead of 8";
}

void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }
  for (const auto& functor : functors) {
    functor();
  }
  callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const {
  for (const auto& channel : activeChannels_) {
    LOG_TRACE << "{" << channel->revents2String() << "}";
  }
}
