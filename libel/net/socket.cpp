//
// Created by kaymind on 2020/12/3.
//

#include "libel/net/socket.h"

#include "libel/base/logging.h"
#include "libel/net/callbacks.h"
#include "libel/net/inet_address.h"
#include "libel/net/sockets_ops.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <cstdio>

using namespace Libel;
using namespace Libel::net;

Socket::~Socket() { sockets::close(sockfd_); }

bool Socket::getTcpInfo(struct tcp_info *tcpi) const {
  socklen_t len = sizeof(*tcpi);
  memZero(tcpi, len);
  return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char *buf, size_t len) const {
  struct tcp_info tcpi {};
  bool ok = getTcpInfo(&tcpi);
  if (ok) {
    snprintf(buf, len,
             "unrecovered=%u "
             "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
             "lost=%u retrans=%u rtt=%u rttvar=%u "
             "sshthresh=%u cwnd=%u total_retrans=%u ",
             tcpi.tcpi_retransmits, tcpi.tcpi_rto, tcpi.tcpi_ato,
             tcpi.tcpi_snd_mss, tcpi.tcpi_rcv_mss, tcpi.tcpi_lost,
             tcpi.tcpi_retrans, tcpi.tcpi_rtt, tcpi.tcpi_rttvar,
             tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
             tcpi.tcpi_total_retrans);
  }
  return ok;
}

void Socket::bindAddress(const InetAddress &local_addr) {
  sockets::bindOrDie(sockfd_, local_addr.getSockAddr());
}

void Socket::listen() {
  sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress *peer_addr) {
  struct sockaddr_in6 addr{};
  memZero(&addr, sizeof(addr));
  int connfd = sockets::accept(sockfd_, &addr);
  if (connfd >= 0) {
    peer_addr->setSockAddrInet6(addr);
  }
  return connfd;
}

void Socket::shutdownWrite() {
  sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on) {
  int opt_val = on ? 1 : 0;
  auto ret = ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &opt_val, static_cast<socklen_t >(sizeof(opt_val)));
  if (ret < 0 && on) {
    LOG_ERROR << "failed to set socket TPC_NODELAY socket:" << sockfd_ << " error:" << strerror(errno);
  }
}

void Socket::setReuseAddr(bool on) {
  int opt_val = on ? 1 : 0;
  auto ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt_val, static_cast<socklen_t >(sizeof(opt_val)));
  if (ret < 0 && on)
    LOG_ERROR << "failed to set socket SO_REUSEADDR socket:" << sockfd_ << " error:" << strerror(errno);
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
  int opt_val = on ? 1 : 0;
  auto ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &opt_val, static_cast<socklen_t >(sizeof(opt_val)));
  if (ret < 0 && on) {
    LOG_ERROR << "failed to set socket SO_REUSEPORT socket:" << sockfd_ << " error:" << strerror(errno);
  }
#else
  if (on) {
    LOG_ERROR << "SO_REUSEPORT is not supported.";
  }
#endif
}

void Socket::setKeepAlive(bool on) {
  int opt_val = on ? 1 : 0;
  auto ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &opt_val, static_cast<socklen_t >(sizeof(opt_val)));
  if (ret < 0 && on)
    LOG_ERROR << "failed to set socket SO_KEEPALIVE socket:" << sockfd_ << " error:" << strerror(errno);
}


