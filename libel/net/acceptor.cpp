//
// Created by kaymind on 2020/12/3.
//

#include "libel/net/acceptor.h"

#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/inet_address.h"
#include "libel/net/sockets_ops.h"

#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

using namespace Libel;
using namespace Libel::net;

Acceptor::Acceptor(Libel::net::EventLoop *loop,
                   const Libel::net::InetAddress &listenAddr, bool reusePort)
    : loop_(loop),
      acceptSocket_(sockets::createNonBlockingSocketOrDie(listenAddr.family())),
      acceptChannel_(loop, acceptSocket_.fd()),
      isListening_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reusePort);
  acceptSocket_.bindAddress(listenAddr);
  acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {
  acceptChannel_.disableAll();
  acceptChannel_.removeSelfFromLoop();
  ::close(idleFd_);
}

void Acceptor::listen() {
  loop_->assertInLoopThread();
  isListening_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
  loop_->assertInLoopThread();
  InetAddress peerAddr;
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0) {
    if (newConnectionCallback_)
      newConnectionCallback_(connfd, peerAddr);
    else
      sockets::close(connfd);
  } else {
    LOG_ERROR << "failed to call accept in Acceptor::handleRead";
    /// refer "The special problem of accept()ing when you cant"
    /// in libev's doc.
    if (errno == EMFILE) {
      /// in case of cpu busy loop, we reserve a idle fd
      /// when process runs out of fd, use this fd as tmp,
      /// but close the connection just after accept
      ::close(idleFd_);
      idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);
      ::close(idleFd_);
      /// not thread safe, if one thread call accept at this moment
      /// ::open will fail.
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

