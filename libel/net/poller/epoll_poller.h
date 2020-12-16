//
// Created by kaymind on 2020/11/26.
//

#ifndef LIBEL_EPOLL_POLLER_H
#define LIBEL_EPOLL_POLLER_H

#include "libel/net/poller.h"

#include <vector>

// forward declaration
struct epoll_event;

namespace Libel {

namespace net {

class EpollPoller : public Poller {
public:
    explicit EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    TimeStamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel *channel) override;
    // acquire: channel->isNoneEvent is true
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    static const char* operation2String(int op);

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    void update(int operation, Channel *channel);

    /// An std::vector<T> instance has a fixed size, no matter what T is
    /// But the vector's constructor depends on the concrete type, so we
    /// declare constructor in this file and implement in *.cc file
    using EventList = std::vector<struct epoll_event>;
    int epollfd_;
    EventList events_;

};

}

}

#endif //LIBEL_EPOLL_POLLER_H
