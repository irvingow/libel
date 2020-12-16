//
// Created by kaymind on 2020/12/5.
//

#ifndef LIBEL_TCP_CLIENT_H
#define LIBEL_TCP_CLIENT_H

#include "libel/base/Mutex.h"
#include "libel/net/tcp_connection.h"

#include <atomic>

namespace Libel {

namespace net {

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable {
public:
  TcpClient(EventLoop* loop, const InetAddress& serverAddr, std::string nameArg);
  ~TcpClient();

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connector() const {
    MutexLockGuard lock(mutex_);
    return connection_;
  }

  EventLoop* getLoop() const { return loop_; }
  bool retry() const { return retry_ == true; }
  void enableRetry() { retry_ = true; }

  const std::string& name() const {
    return name_;
  }

  /// Set connection callback
  /// not thread safe
  void setConnectionCallback(ConnectionCallback cb) {
    connectionCallback_ = std::move(cb);
  }

  /// Set message callback
  /// not thread safe
  void setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
  }

  /// Set write complete callback
  /// not thread safe
  void setWriteCompleteCallback(WriteCompleteCallback cb) {
    writeCompleteCallback_ = std::move(cb);
  }


private:
  /// not thead safe, but in loop
  void newConnection(int sockfd);
  /// not thead safe, but in loop
  void removeConnection(const TcpConnectionPtr& conn);

private:
  EventLoop* loop_;
  ConnectorPtr connector_;
  const std::string name_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  std::atomic_bool retry_;
  std::atomic_bool connect_;
  int nextConnId_;
  mutable MutexLock mutex_;
  TcpConnectionPtr connection_ GUARDED_BY(mutex_);
};

}
}

#endif //LIBEL_TCP_CLIENT_H
