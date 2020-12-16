//
// Created by kaymind on 2020/11/26.
//

#ifndef LIBEL_POLLER_H
#define LIBEL_POLLER_H

#include <map>
#include <vector>

#include "libel/base/timestamp.h"
#include "libel/net/eventloop.h"

namespace Libel {

namespace net {

class Channel;

/// base class for IO Multiplexing
class Poller : noncopyable {
 public:
    using ChannelList = std::vector<Channel *>;

    explicit Poller(EventLoop *loop);
    virtual ~Poller() = default;

    virtual TimeStamp poll(int timeoutMs, ChannelList* activeChannles) = 0;

    virtual void updateChannel(Channel *channel) = 0;

    virtual void removeChannel(Channel *channel) = 0;

    virtual bool hasChannel(Channel *channel) const;

    static Poller* newDefaultPoller(EventLoop *loop);

    void assertInLoopThread() const {
        ownerLoop_->assertInLoopThread();
    }

 protected:
  using ChannelMap = std::map<int, Channel*>;
  ChannelMap channelMap_;

 private:
  EventLoop* ownerLoop_;
};

}  // namespace net

}  // namespace Libel

#endif  // LIBEL_POLLER_H
