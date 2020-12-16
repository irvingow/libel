//
// Created by kaymind on 2020/11/26.
//

#include "libel/net/poller/poll_poller.h"

#include "libel/base/logging.h"
#include "libel/net/channel.h"
#include "libel/net/callbacks.h"

#include <cassert>
#include <cerrno>
#include <sys/poll.h>

using namespace Libel;
using namespace Libel::net;

PollPoller::PollPoller(Libel::net::EventLoop *loop) : Poller(loop) {}

TimeStamp PollPoller::poll(int timeoutMs, ChannelList *activeChannels) {
    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs);
    TimeStamp now(TimeStamp::now());

    if (numEvents > 0) {
        LOG_TRACE << numEvents << " events happened";
        fillActiveChannels(numEvents, activeChannels);
    } else if (numEvents == 0) {
        LOG_TRACE << " no event happened";
    } else {
        if (errno != EINTR) {
            LOG_ERROR << "pollPoller::poll() failed error:" << strerror(errno);
        }
    }
    return now;
}

void PollPoller::fillActiveChannels(int numOfEvents, Libel::net::Poller::ChannelList *activeChannles) const {
    for (auto iter = pollfds_.begin(); iter != pollfds_.end() && numOfEvents > 0; ++iter) {
        if (iter->revents > 0) {
            --numOfEvents;
            auto channel_iter = channelMap_.find(iter->fd);
            assert(channel_iter != channelMap_.end());
            auto channel = channel_iter->second;
            assert(channel->fd() == iter->fd);
            channel->set_revent(iter->revents);
            activeChannles->push_back(channel);
        }
    }
}

void PollPoller::updateChannel(Channel *channel) {
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    if (channel->get_index() < 0) {
        /// a new fd, add to pollfds
        assert(channelMap_.find(channel->fd()) == channelMap_.end());
        struct pollfd pfd{};
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        int idx = static_cast<int>(pollfds_.size());
        pollfds_.push_back(pfd);
        channel->set_index(idx);
        channelMap_[pfd.fd] = channel;
    } else {
        /// a existing one, need to be updated
        assert(channelMap_.find(channel->fd()) != channelMap_.end());
        assert(channelMap_[channel->fd()] == channel);
        int idx = channel->get_index();
        assert(idx >= 0 && idx < static_cast<int>(pollfds_.size()));
        auto &pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent()) {
            /// ::poll will ignore fd whose value is negative
            /// minus 1 because fd can be zero
            pfd.fd = -channel->fd() - 1;
        }
    }
}

void PollPoller::removeChannel(Channel *channel) {
    Poller::assertInLoopThread();
    LOG_TRACE << "remove fd = " << channel->fd();
    assert(channelMap_.find(channel->fd()) != channelMap_.end());
    assert(channelMap_[channel->fd()] == channel);
    assert(channel->isNoneEvent());
    auto idx = channel->get_index();
    assert(idx >= 0 && idx < static_cast<int>(pollfds_.size()));
    auto n = channelMap_.erase(channel->fd());
    assert(n == 1);
    (void)n;
    if (implicit_cast<size_t>(idx) == pollfds_.size() - 1) {
        pollfds_.pop_back();
    } else {
        auto channelFdAtEnd = pollfds_.back().fd;
        std::iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
        if (channelFdAtEnd < 0) {
            channelFdAtEnd = -channelFdAtEnd - 1;
        }
        channelMap_[channelFdAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}









