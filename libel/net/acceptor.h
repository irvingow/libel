//
// Created by kaymind on 2020/12/3.
//

#ifndef LIBEL_ACCEPTOR_H
#define LIBEL_ACCEPTOR_H

#include "libel/net/channel.h"
#include "libel/net/sockets_ops.h"
#include "libel/net/socket.h"

#include <functional>

namespace Libel {

namespace net {

class EventLoop;
class InetAddress;

/// Acceptor of incoming TCP connections.
class Acceptor : noncopyable {
public:
  using NewConnectionCallback = std::function<void (int sockfd, const InetAddress&)>;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reusePort);
  ~Acceptor();

  void setNewConnectionCallback(NewConnectionCallback cb) {
    newConnectionCallback_ = std::move(cb);
  }

  void listen();

  bool isListening() const { return isListening_; }

private:
  void handleRead();

  EventLoop* loop_;
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool isListening_;
  int idleFd_;
};

}
}

#endif //LIBEL_ACCEPTOR_H
