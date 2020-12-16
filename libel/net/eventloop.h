//
// Created by kaymind on 2020/11/25.
//

#ifndef LIBEL_EVENTLOOP_H
#define LIBEL_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>

#include "libel/base/Mutex.h"
#include "libel/base/current_thread.h"
#include "libel/base/timestamp.h"
#include "libel/net/callbacks.h"
#include "libel/net/timerId.h"

namespace Libel {

namespace net {

class Channel;
class Poller;
class TimerQueue;

/// Reactor, at most one per thread
class EventLoop : noncopyable {
 public:
  using Functor = std::function<void()>;
  using ChannelList = std::vector<Channel *>;

  EventLoop();
  ~EventLoop();  // for std::unique_ptr members

  /// loops forever.
  /// should be called in the same thread as creation of the loop
  void loop();

  /// quits loop
  ///
  void quit();

  /// time when poller returns, usually means data arrival
  TimeStamp pollReturnTime() const { return pollReturnTime_; }

  int64_t iteration() const { return iteration_; }

  /// thread safe
  void runInLoop(Functor cb);

  /// thread safe
  void queueInLoop(Functor cb);

  size_t queueSize() const;

  // timers

  ///
  /// Runs callback at 'time'
  /// Thread Safe
  ///
  TimerId runAt(TimeStamp time, TimerCallback cb);
  ///
  /// Runs callback after delay seconds.
  /// Thread Safe
  ///
  TimerId runAfter(double delaySeconds, TimerCallback cb);
  ///
  /// Run callback every interval seconds.
  /// Thread Safe
  ///
  TimerId runEvery(double intervalSeconds, TimerCallback cb);
  ///
  /// Cancels the timer.
  /// Thread Safe
  ///
  void cancel(const TimerId &timerId);

  void wakeup();

  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  bool hasChannel(Channel *channel);

  void assertInLoopThread() {
    if (!isInLoopThread()) {
      abortNotInLoopThread();
    }
  }

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  bool eventHandling() const { return eventHandling_; }

  void setContext(void *context) { context_ = context; }

  static EventLoop *getEventLoopOfCurrentThead();

 private:
  void abortNotInLoopThread();
  void handleRead();  // for wakeup
  void doPendingFunctors();

  void printActiveChannels() const;  // just for debug

  std::atomic<bool> looping_;
  std::atomic<bool> quit_;
  std::atomic<bool> eventHandling_;
  std::atomic<bool> callingPendingFunctors_;
  int64_t iteration_;
  const pid_t threadId_;
  TimeStamp pollReturnTime_;
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue> timerQueue_;
  int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;
  void *context_;

  ChannelList activeChannels_;
  Channel *currentActiveChannel_;

  mutable MutexLock mutex_;
  std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);
};

}  // namespace net

}  // namespace Libel

#endif  // LIBEL_EVENTLOOP_H
