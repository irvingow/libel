//
// Created by kaymind on 2020/11/26.
//

#include "libel/net/poller.h"
#include "libel/net/poller/poll_poller.h"
#include "libel/net/poller/epoll_poller.h"

using namespace Libel::net;

Poller * Poller::newDefaultPoller(EventLoop *loop) {
    if (::getenv("LIBEL_USE_POLL"))
        return new PollPoller(loop);
    else
        return new EpollPoller(loop);
}