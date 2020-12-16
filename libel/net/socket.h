//
// Created by kaymind on 2020/12/3.
//

#ifndef LIBEL_SOCKET_H
#define LIBEL_SOCKET_H

#include "libel/base/noncopyable.h"

#include <cstddef>

// forward declaration
// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace Libel {

namespace net {

class InetAddress;

/// Wrapper of socket descriptor
class Socket : noncopyable {
public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}

  /// close socket fd when destructs
  ~Socket();

  int fd() const { return sockfd_; }

  bool getTcpInfo(struct tcp_info*) const;
  bool getTcpInfoString(char *buf, size_t len) const;

  /// abort if address in use
  void bindAddress(const InetAddress& local_addr);
  /// abort if address in use
  void listen();

  /// on success, returns a non-negative integer that
  /// is a descriptor for the accepted socket, which
  /// has been set to non-blocking and close-on-exec,
  /// and *peer_addr is assigned.
  /// on error, -1 is returned, *peer_addr is untouched.
  int accept(InetAddress* peer_addr);

  void shutdownWrite();

  /// enable/disable TCP_NODELAY( Tcp Nagle's algorithm)
  void setTcpNoDelay(bool on);

  /// enable/disable SO_REUSEADDR
  void setReuseAddr(bool on);
  /// enable/disable SO_REUSEPORT
  void setReusePort(bool on);

  /// enable/disable SO_KEEPALIVE
  void setKeepAlive(bool on);

private:
  const int sockfd_;
};

}
}

#endif //LIBEL_SOCKET_H
