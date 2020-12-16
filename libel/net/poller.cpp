//
// Created by kaymind on 2020/11/26.
//

#include "libel/net/poller.h"

#include "libel/net/channel.h"

using namespace Libel;
using namespace Libel::net;

Poller::Poller(EventLoop *loop) : ownerLoop_(loop) {}

bool Poller::hasChannel(Channel *channel) const {
    assertInLoopThread();
    auto iter = channelMap_.find(channel->fd());
    return iter != channelMap_.end() && iter->second == channel;
}