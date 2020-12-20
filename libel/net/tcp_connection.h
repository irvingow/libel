//
// Created by kaymind on 2020/12/5.
//

#ifndef LIBEL_TCP_CONNECTION_H
#define LIBEL_TCP_CONNECTION_H

#include "libel/base/noncopyable.h"
#include "libel/net/buffer.h"
#include "libel/net/callbacks.h"
#include "libel/net/inet_address.h"

#include <memory>

/// forward declaration
/// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace Libel {

namespace net {

class Channel;
class EventLoop;
class Socket;

///
/// Tcp connection, for both client and server usage
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object
  TcpConnection(EventLoop* loop, std::string name, int sockfd,
                const InetAddress& localAddr, const InetAddress& peerAddr);
  ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }
  const std::string& name() const { return name_; }
  const InetAddress& localAddress() const { return localAddr_; }
  const InetAddress& peerAddress() const { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }
  bool disconnected() const { return state_ == kDisconnected; }
  // return true if success
  bool getTcpInfo(struct tcp_info*) const;
  std::string getTcpInfoString() const;

  void send(const void* message, int len);
  void send(const std::string& message);
  void send(Buffer* message);  // this one will swap data
  void shutdown();             // NOT thread safe, but no simultaneous calling
  void forceClose();
  void forceCloseWithDelay(double seconds);
  void setTcpNoDelay(bool on);
  void startRead();
  void stopRead();
  bool isReading() const {
    return reading_;
  }  // NOT thread safe, may race with start/stopReadInLoop

  void setContext(std::shared_ptr<void> context) { context_ = context; }

  const std::shared_ptr<void>& getContext() const { return context_; }

  void setConnectionCallback(ConnectionCallback cb) {
    connectionCallback_ = std::move(cb);
  }

  void setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
  }

  void setWriteCompleteCallback(WriteCompleteCallback cb) {
    writeCompleteCallback_ = std::move(cb);
  }

  void setHighWaterMarkCallback(HighWaterMarkCallback cb,
                                size_t highWaterMark) {
    highWaterMarkCallback_ = std::move(cb);
    highWaterMark_ = highWaterMark;
  }

  void setCloseCallback(CloseCallback cb) { closeCallback_ = std::move(cb); }

  /// called when TcpServer accepts a new connection
  void connectEstablished();  /// should be called only once
  /// called when TcpClient has removed self from its map
  void connectDestroyed();  /// should be called only once

  /// for some special usage
  Buffer* inputBuffer() { return &inputBuffer_; }

  Buffer* outputBuffer() { return &outputBuffer_; }

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void handleRead(TimeStamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();
  void sendInLoop(const std::string& message);
  void sendInLoop(const void* message, size_t len);
  void shutdownInLoop();
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char* stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

 private:
  EventLoop* loop_;
  const std::string name_;
  StateE state_;
  bool reading_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress localAddr_;
  const InetAddress peerAddr_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
  size_t highWaterMark_;
  Buffer inputBuffer_;
  Buffer outputBuffer_;
  std::shared_ptr<void> context_;
};

}  // namespace net
}  // namespace Libel

#endif  // LIBEL_TCP_CONNECTION_H
