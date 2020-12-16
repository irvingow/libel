//
// Created by kaymind on 2020/12/1.
//

#include "libel/net/sockets_ops.h"
#include "libel/base/logging.h"
#include "libel/net/Endian.h"
#include "libel/net/callbacks.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>

using namespace Libel;
using namespace Libel::net;

const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in6 *addr) {
  return static_cast<const struct sockaddr *>(
      implicit_cast<const void *>(addr));
}

struct sockaddr *sockets::sockaddr_cast(struct sockaddr_in6 *addr) {
  return static_cast<struct sockaddr *>(implicit_cast<void *>(addr));
}

const struct sockaddr *sockets::sockaddr_cast(const struct sockaddr_in *addr) {
  return static_cast<const struct sockaddr *>(
      implicit_cast<const void *>(addr));
}

const struct sockaddr_in *sockets::sockaddr_in_cast(
    const struct sockaddr *addr) {
  return static_cast<const struct sockaddr_in *>(
      implicit_cast<const void *>(addr));
}

const struct sockaddr_in6 *sockets::sockaddr_in6_cast(
    const struct sockaddr *addr) {
  return static_cast<const struct sockaddr_in6 *>(
      implicit_cast<const void *>(addr));
}

void sockets::setNonBlockingAndCloseOnExecOrDie(int sockfd) {
  /// set non blocking
  int flags = ::fcntl(sockfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd, F_SETFL, flags);
  if (ret < 0) {
    LOG_FATAL
        << "sockets::setNonBlockingAndCloseOnExecOrDie set non blocking error:" << strerror(errno);
  }
  /// set close on exec
  flags = ::fcntl(sockfd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd, F_SETFD, flags);
  if (ret < 0) {
    LOG_FATAL
        << "sockets::setNonBlockingAndCloseOnExecOrDie set close on exec error:" << strerror(errno);
  }
}

int sockets::createNonBlockingSocketOrDie(sa_family_t family) {
  int sockfd =
      ::socket(family, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_IP);
  if (sockfd < 0) {
    LOG_FATAL << "sockets::createNonBlockingOrDie error:" << strerror(errno);
  }
  return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr *addr) {
  int ret =
      ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
  if (ret < 0) {
    LOG_FATAL << "sockets::bindOrDie error:" << strerror(errno);
  }
}

void sockets::listenOrDie(int sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    LOG_FATAL << "sockets::listenOrDie error:" << strerror(errno);
  }
}

int sockets::accept(int sockfd, struct sockaddr_in6 *addr) {
  auto addrlen = static_cast<socklen_t>(sizeof *addr);
  int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
  setNonBlockingAndCloseOnExecOrDie(connfd);
  if (connfd < 0) {
    int savedErrno = errno;
    LOG_ERROR << "Socket::accept";
    switch (savedErrno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO:  // ???
      case EPERM:
      case EMFILE:  // per-process limit of open file descriptor ???
        // expected errors
        errno = savedErrno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        LOG_FATAL << "unexpected error of ::accept error:" << strerror(savedErrno);
        break;
      default:
        LOG_FATAL << "unknown error of ::accept error:" << strerror(savedErrno);
        break;
    }
  }
  return connfd;
}

int sockets::connect(int sockfd, const struct sockaddr *addr) {
  return ::connect(sockfd, addr,
                   static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::read(int sockfd, void *buf, size_t count) {
  return ::read(sockfd, buf, count);
}

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt) {
  return ::readv(sockfd, iov, iovcnt);
}

ssize_t sockets::write(int sockfd, const void *buf, size_t count) {
  return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd) {
  if (::close(sockfd) < 0) {
    LOG_ERROR << "sockets::close failed";
  }
}

void sockets::shutdownWrite(int sockfd) {
  if (::shutdown(sockfd, SHUT_WR) < 0) {
    LOG_ERROR << "sockets::shutdownWrite failed";
  }
}

void sockets::toIpPort(char *buf, size_t size, const struct sockaddr *addr) {
  toIp(buf, size, addr);
  auto end = ::strlen(buf);
  const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
  uint16_t port = sockets::networkToHost16(addr4->sin_port);
  assert(size > end);
  snprintf(buf + end, size - end, ":%u", port);
}

void sockets::toIp(char *buf, size_t size, const struct sockaddr *addr) {
  if (addr->sa_family == AF_INET) {
    assert(size >= INET_ADDRSTRLEN);
    const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
    ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
  } else if (addr->sa_family == AF_INET6) {
    assert(size >= INET6_ADDRSTRLEN);
    const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
    ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
  }
}

void sockets::fromIpPort(const char *ip, uint16_t port,
                         struct sockaddr_in *addr) {
  addr->sin_family = AF_INET;
  addr->sin_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    LOG_ERROR << "sockets::fromIpPort call inet_pton error";
}

void sockets::fromIpPort(const char *ip, uint16_t port,
                         struct sockaddr_in6 *addr) {
  addr->sin6_family = AF_INET6;
  addr->sin6_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0)
    LOG_ERROR << "sockets::fromIpPort call inet_pton error";
}

int sockets::getSocketError(int sockfd) {
  int opt_val = -1;
  auto opt_len = static_cast<socklen_t>(sizeof(opt_val));
  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &opt_val, &opt_len) < 0)
    return errno;
  else
    return opt_val;
}

struct sockaddr_in6 sockets::getLocalAddr(int sockfd) {
  struct sockaddr_in6 local_addr;
  memZero(&local_addr, sizeof(local_addr));
  auto addr_len = static_cast<socklen_t>(sizeof(local_addr));
  if (::getsockname(sockfd, sockaddr_cast(&local_addr), &addr_len) < 0)
    LOG_ERROR << "sockets::getLocalAddr error";
  return local_addr;
}

struct sockaddr_in6 sockets::getPeerAddr(int sockfd) {
  struct sockaddr_in6 peer_addr;
  memZero(&peer_addr, sizeof(peer_addr));
  auto addr_len = static_cast<socklen_t>(sizeof(peer_addr));
  if (::getpeername(sockfd, sockaddr_cast(&peer_addr), &addr_len) < 0)
    LOG_ERROR << "sockets::getPeerAddr error";
  return peer_addr;
}

bool sockets::isSelfConnect(int sockfd) {
  struct sockaddr_in6 local_addr = getLocalAddr(sockfd);
  struct sockaddr_in6 peer_addr = getPeerAddr(sockfd);
  if (local_addr.sin6_family == AF_INET) {
    const struct sockaddr_in *l =
        reinterpret_cast<struct sockaddr_in *>(&local_addr);
    const struct sockaddr_in *p =
        reinterpret_cast<struct sockaddr_in *>(&peer_addr);
    return l->sin_port == p->sin_port &&
           l->sin_addr.s_addr == p->sin_addr.s_addr;
  } else if (local_addr.sin6_family == AF_INET6) {
    return local_addr.sin6_port == peer_addr.sin6_port &&
           memcmp(&local_addr.sin6_addr, &peer_addr.sin6_addr,
                  sizeof(local_addr.sin6_addr)) == 0;
  }
  return false;
}
