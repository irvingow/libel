//
// Created by kaymind on 2020/11/26.
//

#include "libel/net/poller/epoll_poller.h"
#include "libel/base/logging.h"
#include "libel/net/channel.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>

using namespace Libel;
using namespace Libel::net;

namespace {

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

const char *index2String(int index) {
  if (index == kNew)
    return "kNew";
  else if (index == kAdded)
    return "kAdded";
  else if (index == kDeleted)
    return "kDeleted";
  else
    return "unknown operation";
}

}  // namespace

EpollPoller::EpollPoller(Libel::net::EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    LOG_FATAL << "EpollPoller::EpollPoller call epoll_create1 failed";
  }
}

EpollPoller::~EpollPoller() { ::close(epollfd_); }

TimeStamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
  LOG_TRACE << "total channels size" << activeChannels->size();
  int numEvents = ::epoll_wait(epollfd_, events_.data(),
                               static_cast<int>(events_.size()), timeoutMs);
  auto now = TimeStamp::now();
  if (numEvents > 0) {
    LOG_TRACE << numEvents << " events happened";
    fillActiveChannels(numEvents, activeChannels);
    if (implicit_cast<size_t>(numEvents) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents == 0) {
    LOG_TRACE << "no events happened";
  } else {
    if (errno != EINTR) {
      LOG_ERROR << "EpollPoller::poll() failed error:" << strerror(errno);
    }
  }
  return now;
}

void EpollPoller::fillActiveChannels(
    int numEvents, Libel::net::Poller::ChannelList *activeChannels) const {
  assert(implicit_cast<size_t>(numEvents) <= events_.size());
  for (int i = 0; i < numEvents; ++i) {
    auto channel = static_cast<Channel *>(events_[i].data.ptr);
    int fd = channel->fd();
    auto iter = channelMap_.find(fd);
    assert(iter != channelMap_.end());
    assert(iter->second == channel);
    (void)iter;
    channel->set_revent(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void EpollPoller::updateChannel(Channel *channel) {
  Poller::assertInLoopThread();
  const int index = channel->get_index();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events()
            << " index = " << index2String(index);
  if (index == kNew || index == kDeleted) {
    int fd = channel->fd();
    if (index == kNew) {
      assert(channelMap_.find(fd) == channelMap_.end());
      channelMap_[fd] = channel;
    } else {
      assert(channelMap_.find(fd) != channelMap_.end());
      assert(channelMap_[fd] == channel);
    }
    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  } else {
    int fd = channel->fd();
    (void)fd;  // prevent compiler complaint
    assert(channelMap_.find(fd) != channelMap_.end());
    assert(channelMap_[fd] == channel);
    assert(index == kAdded);
    if (channel->isNoneEvent()) {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    } else {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EpollPoller::removeChannel(Channel *channel) {
  Poller::assertInLoopThread();
  int fd = channel->fd();
  LOG_TRACE << "remove channel fd:" << fd;
  assert(channelMap_.find(fd) != channelMap_.end());
  assert(channelMap_[fd] == channel);
  assert(channel->isNoneEvent());
  int index = channel->get_index();
  size_t n = channelMap_.erase(fd);
  assert(n == 1);
  (void)n;
  if (index == kAdded) {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

void EpollPoller::update(int operation, Libel::net::Channel *channel) {
  struct epoll_event event {};
  memZero(&event, sizeof(event));
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  LOG_TRACE << "epoll_ctl op = " << operation2String(operation) << " event = { "
            << Channel::events2String(fd, channel->events()) << "}";
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_ERROR << "epoll_ctl op = " << operation2String(operation)
                << "fd = " << fd;
    } else {
      LOG_FATAL << "epoll_ctl op = " << operation2String(operation)
                << "fd = " << fd;
    }
  }
}

const char *EpollPoller::operation2String(int op) {
  switch (op) {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_MOD:
      return "MOD";
    case EPOLL_CTL_DEL:
      return "DEL";
    default:
      assert(false && "ERROR operation");  /// for error message
      return "Unknown Operation";
  }
}
