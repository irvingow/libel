//
// Created by kaymind on 2020/12/5.
//

#include "libel/net/tcp_connection.h"

#include "libel/base/logging.h"
#include "libel/net/callbacks.h"
#include "libel/net/channel.h"
#include "libel/net/socket.h"
#include "libel/net/sockets_ops.h"
#include "libel/net/eventloop.h"

#include <cerrno>

using namespace Libel;
using namespace Libel::net;

void Libel::net::defaultConnectionCallback(const TcpConnectionPtr &conn) {
  LOG_TRACE << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << "is"
            << (conn->connected() ? "Up" : "Down");
}

void Libel::net::defaultMessageCallback(const TcpConnectionPtr &conn,
                                        Buffer *buffer, TimeStamp receiveTime) {
  buffer->retrieveAll();
}

TcpConnection::TcpConnection(Libel::net::EventLoop *loop, std::string name,
                             int sockfd,
                             const Libel::net::InetAddress &localAddr,
                             const Libel::net::InetAddress &peerAddr)
    : loop_(loop),
      name_(std::move(name)),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024),
      context_(nullptr) {
  assert(loop != nullptr);
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
  LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
  << " fd = " << sockfd;
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
  << " fd = " << channel_->fd() << " state = " << stateToString();
  assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info *tcpInfo) const {
  return socket_->getTcpInfo(tcpInfo);
}

std::string TcpConnection::getTcpInfoString() const {
  char buf[1024] = {};
  buf[0] = '\0';
  socket_->getTcpInfoString(buf, sizeof(buf));
  return buf;
}

void TcpConnection::send(const void *message, int len) {
  send(std::string(static_cast<const char*>(message), len));
}

void TcpConnection::send(const std::string &message) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      void (TcpConnection::*fp)(const std::string& message) = &TcpConnection::sendInLoop;
      loop_->queueInLoop(std::bind(fp, this, message));
    }
  }
}

void TcpConnection::send(Buffer *message) {
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message->peek(), message->readableBytes());
      message->retrieveAll();
    } else {
      void (TcpConnection::*fp)(const std::string& message) = &TcpConnection::sendInLoop;
      loop_->runInLoop(std::bind(fp, this, message->retrieveAllAsString()));
    }
  }
}

void TcpConnection::sendInLoop(const std::string &message) {
  sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void *message, size_t len) {
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool fatalError = false;
  if (state_ == kDisconnected) {
    LOG_WARN << "disconnected, give up writing";
    return;
  }
  /// if nothing in output queue, try writing data directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = sockets::write(channel_->fd(), message, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
        loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_ERROR << " failed to call send in TcpConnection::sendInLoop";
        if (errno == EPIPE || errno == ECONNRESET) // FIXME: any other?
          fatalError = true;
      }
    }
  }
  assert(remaining <= len);
  if (!fatalError && remaining > 0) {
    size_t  oldlen = outputBuffer_.readableBytes();
    if (oldlen + remaining >= highWaterMark_ && oldlen < highWaterMark_
    && highWaterMarkCallback_) {
      loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaining));
    }
    outputBuffer_.append(static_cast<const char*>(message) + nwrote, remaining);
    if (!channel_->isWriting())
      channel_->enableWriting();
  }
}

void TcpConnection::shutdown() {
  if (state_ == kConnected) {
    setState(kDisconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  if (!channel_->isWriting()) {
    // we are not gonna write
    socket_->shutdownWrite();
  }
}

void TcpConnection::forceClose() {
  if (state_ == kConnected || state_ == kDisconnecting) {
    setState(kDisconnecting);
    loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop() {
  loop_->assertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting) {
    // as if we received 0 byte in handleRead().
    handleClose();
  }
}

const char* TcpConnection::stateToString() const {
  switch (state_) {
    case kDisconnected:
      return "kDisconnected";
    case kDisconnecting:
      return "kDisconnecting";
    case kConnected:
      return "kConnected";
    case kConnecting:
      return "kConnecting";
    default:
      return "unknown state";
  }
}

void TcpConnection::setTcpNoDelay(bool on) {
  socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead() {
  loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop() {
  loop_->assertInLoopThread();
  if (!reading_ || !channel_->isReading()) {
    channel_->enableReading();
    reading_ = true;
  }
}

void TcpConnection::stopRead() {
  loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop() {
  loop_->assertInLoopThread();
  if (reading_ || channel_->isReading()) {
    channel_->disableReading();
    reading_ = false;
  }
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  if (state_ == kConnected) {
    setState(kDisconnected);
    channel_->disableAll();

    connectionCallback_(shared_from_this());
  }
  channel_->removeSelfFromLoop();
}

void TcpConnection::handleRead(Libel::TimeStamp receiveTime) {
  loop_->assertInLoopThread();
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0) {
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  } else if (n == 0) {
    handleClose();
  } else {
    errno = savedErrno;
    LOG_ERROR << "failed to read message TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = sockets::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
    if (n > 0) {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWriting();
        if (writeCompleteCallback_)
          loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
        if (state_ == kDisconnecting)
          shutdownInLoop();
      }
    } else {
      LOG_ERROR << " failed to call write";
    }
  } else {
    LOG_TRACE << " Connection fd = " << channel_->fd()
    << " is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  LOG_TRACE << "fd = " << channel_->fd() << " state = " << stateToString();
  assert(state_ == kConnected || state_ == kDisconnecting);
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);
  closeCallback_(guardThis);
}

void TcpConnection::handleError() {
  int err = sockets::getSocketError(channel_->fd());
  LOG_ERROR << "TcpConnection::handleError [" << name_
  << "] - SO_ERROR = " << err << " " << strerror(err);
}

















