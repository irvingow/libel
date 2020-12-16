//
// Created by kaymind on 2020/12/9.
//

#include "libel/net/tcp_server.h"

#include "libel/base/logging.h"
#include "libel/net/acceptor.h"
#include "libel/net/eventloop.h"
#include "libel/net/eventloop_threadpool.h"
#include "libel/net/sockets_ops.h"

#include <cstdio>

using namespace Libel;
using namespace Libel::net;

TcpServer::TcpServer(Libel::net::EventLoop *loop,
                     const Libel::net::InetAddress &listenAddr,
                     std::string nameArg,
                     Libel::net::TcpServer::Option option)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(std::move(nameArg)),
      acceptor_(new Acceptor(loop_, listenAddr, option == kReusePort)),
      threadPool_(new EventLoopThreadPool(loop_, name_)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      started_(ATOMIC_FLAG_INIT),
      nextConnId_(1) {
  acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
  started_.clear();
}

TcpServer::~TcpServer() {
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (auto& item : connections_) {
    TcpConnectionPtr conn(item.second);
    item.second.reset();
    conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  }
}

void TcpServer::setThreadNum(int numTheads) {
  assert(0 <= numTheads);
  threadPool_->setThreadNum(numTheads);
}

void TcpServer::start() {
  if (!started_.test_and_set()) {
    threadPool_->start(threadInitCallback_);
    assert(!acceptor_->isListening());
    loop_->runInLoop(std::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

void TcpServer::newConnection(int sockfd, const Libel::net::InetAddress &peerAddr) {
  loop_->assertInLoopThread();
  auto ioLoop = threadPool_->getNextLoop();
  char buf[64] = {};
  snprintf(buf, sizeof(buf), "-%s#%d", ipPort_.c_str(), nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_
      << "] - new connection [" << connName
      << "] from " << peerAddr.toIpPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
  ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const Libel::net::TcpConnectionPtr &conn) {
  loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const Libel::net::TcpConnectionPtr &conn) {
  loop_->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
      << "] - connection" << conn->name();
  size_t n = connections_.erase(conn->name());
  (void)n;
  assert(n == 1);
  auto ioLoop = conn->getLoop();
  ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}




















