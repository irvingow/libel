//
// Created by kaymind on 2020/11/25.
//

#include "libel/net/channel.h"
#include "libel/base/logging.h"
#include "libel/net/eventloop.h"

#include <cassert>
#include <sstream>

using namespace Libel;
using namespace Libel::net;

Channel::Channel(Libel::net::EventLoop *loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      logHup_(true),
      tied_(false),
      eventHandling_(false),
      addedToLoop_(false) {}

Channel::~Channel() {
  assert(!eventHandling_);
  assert(!addedToLoop_);
  if (loop_->isInLoopThread()) {
    assert(!loop_->hasChannel(this));
  }
}

void Channel::tie(const std::shared_ptr<void> &obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::update() {
  addedToLoop_ = true;
  loop_->updateChannel(this);
}

void Channel::removeSelfFromLoop() {
  assert(isNoneEvent());
  addedToLoop_ = false;
  loop_->removeChannel(this);
}

void Channel::handleEvent(TimeStamp receiveTime) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) handleEventWithGuard(receiveTime);
  } else {
    handleEventWithGuard(receiveTime);
  }
}

/*
 * POLLUP means the socket is no longer connected. In TCP, this means FIN has been received
 * and sent.
 *
 * POLLERR means the socket got an asynchronous error. In TCP, this typically means a RST
 * has been received or sent. if the file descriptor is not socket, POLLERR might mean the
 * device does not support polling.
 *
 * POLLNVAL means the socket file descriptor is not open. It would be an error to close it.
 *
 * POLLPRI means there is some exceptional condition on the file descriptor.
 *
 * POLLRDHUP(since Linux 2.6.17) means stream socket peer closed connection, or shut down
 * writing half of connection
*/
/// \param timeStamp receive event time
void Channel::handleEventWithGuard(Libel::TimeStamp timeStamp) {
  eventHandling_ = true;
  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    if (logHup_) {
      LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
    }
    if (closeCallBack_) {
      closeCallBack_();
    }
  }
  if (revents_ & POLLNVAL) {
    LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
  }
  if (revents_ & (POLLERR | POLLNVAL))
    if (errorCallBack_) errorCallBack_();
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    if (readEventCallBack_) readEventCallBack_(timeStamp);
  if (revents_ & POLLOUT)
    if (writeCallBack_) writeCallBack_();
  eventHandling_ = false;
}

std::string Channel::revents2String() const {
    return events2String(fd(), revents_);
}

std::string Channel::events2String(int fd, int ev) {
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN)
        oss << "IN ";
    if (ev & POLLPRI)
        oss << "PRI ";
    if (ev & POLLOUT)
        oss << "OUT ";
    if (ev & POLLHUP)
        oss << "HUP ";
    if (ev & POLLRDHUP)
        oss << "RDHUP ";
    if (ev & POLLERR)
        oss << "ERR ";
    if (ev & POLLNVAL)
        oss << "NVAL ";
    return oss.str();
}

