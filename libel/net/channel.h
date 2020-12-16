//
// Created by kaymind on 2020/11/25.
//

#ifndef LIBEL_CHANNEL_H
#define LIBEL_CHANNEL_H

#include "libel/base/noncopyable.h"
#include "libel/base/timestamp.h"

#include <functional>
#include <memory>
#include <poll.h>

namespace Libel {

namespace net {

class EventLoop;

class Channel : noncopyable {
public:
    using EventCallBack = std::function<void()>;
    using ReadEventCallBack = std::function<void(TimeStamp)>;

    /// notice that this class doesn't own the file descriptor
    /// The file descriptor could be a socket,
    /// an eventfd, a timerfd or a signal fd.
    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent(TimeStamp receiveTime);
    void setReadCallback(ReadEventCallBack cb) {
        readEventCallBack_ = std::move(cb);
    }
    void setWriteCallback(EventCallBack cb) {
        writeCallBack_ = std::move(cb);
    }
    void setCloseCallback(EventCallBack cb) {
        closeCallBack_ = std::move(cb);
    }
    void setErrorCallback(EventCallBack cb) {
        errorCallBack_ = std::move(cb);
    }

    /// tie this channel to the owner object managed by shared_ptr
    /// avoid the owner object being destroyed when handling event
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revent(int revent) { revents_ = revent; } /// used by poller
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    void enableReading() { events_  |= kReadEvent; update(); }
    void disableReading() { events_  &= ~kReadEvent; update(); }
    void enableWriting() { events_  |= kWriteEvent; update(); }
    void disableWriting() { events_  &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    int get_index() const { return index_; }
    void set_index(int action) { index_ = action; } /// used by poller

    void doNotLogHup() { logHup_ = false; }

    EventLoop* ownerLoop() { return loop_; }

    void removeSelfFromLoop();

    static std::string events2String(int fd, int ev);

    std::string revents2String() const;
private:

    void update();
    void handleEventWithGuard(TimeStamp timeStamp);

    static const int kNoneEvent= 0;
    static const int kReadEvent = POLLIN | POLLPRI;
    static const int kWriteEvent = POLLOUT;

    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_; // received event types of epoll or poll

    // used by Poller to record, -1 means not added by Poller,
    // >= 0 means the index of the channel in Poller
    int index_;
    bool logHup_; // whether to log warning message when received POLLHUP event

    std::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling_;
    bool addedToLoop_;
    ReadEventCallBack readEventCallBack_;
    EventCallBack writeCallBack_;
    EventCallBack closeCallBack_;
    EventCallBack errorCallBack_;
};

}

}

#endif //LIBEL_CHANNEL_H
