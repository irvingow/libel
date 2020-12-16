//
// Created by kaymind on 2020/11/26.
//

#ifndef LIBEL_POLL_POLLER_H
#define LIBEL_POLL_POLLER_H

#include "libel/net/poller.h"

#include <vector>

// forward declaration
struct pollfd;

namespace Libel {

namespace net {

class PollPoller : public Poller {
public:
    explicit PollPoller(EventLoop* loop);
    ~PollPoller() override = default;

    TimeStamp poll(int timeoutMs, ChannelList* activeChannels) override;

    void updateChannel(Channel *channel) override;
    // acquire: channel->isNoneEvent is true
    void removeChannel(Channel *channel) override;

private:
    void fillActiveChannels(int numOfEvents, ChannelList *activeChannels) const;

    /// An std::vector<T> instance has a fixed size, no matter what T is
    /// But the vector's constructor depends on the concrete type, so we
    /// declare constructor in this file and implement in *.cc file
    using PollFdList = std::vector<struct pollfd>;
    PollFdList pollfds_;
};

}

}


#endif //LIBEL_POLL_POLLER_H
