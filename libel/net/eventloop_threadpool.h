//
// Created by kaymind on 2020/12/3.
//

#ifndef LIBEL_EVENTLOOP_THREADPOOL_H
#define LIBEL_EVENTLOOP_THREADPOOL_H

#include "libel/base/noncopyable.h"
#include "libel/net/callbacks.h"

#include <functional>
#include <memory>
#include <vector>

namespace Libel {

namespace net {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable {
public:
  using ThreadInitCallback = std::function<void (EventLoop*)>;
  EventLoopThreadPool(EventLoop* baseLoop, std::string name);
  // we can't use default because forward declaration and std::unique_ptr
  ~EventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start(const ThreadInitCallback& cb = ThreadInitCallback());

  // round-robin policy
  EventLoop* getNextLoop();

  // with the same hash code, it will always return the same eventloop
  EventLoop* getLoopForHash(size_t hashCode);

  std::vector<EventLoop*> getAllLoops();

  bool started() const { return started_; }

  const std::string &name() const { return name_; }

private:

  EventLoop* baseLoop_;
  std::string name_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;
};


}
}

#endif //LIBEL_EVENTLOOP_THREADPOOL_H
