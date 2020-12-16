//
// Created by kaymind on 2020/12/9.
//

#ifndef LIBEL_TCP_SERVER_H
#define LIBEL_TCP_SERVER_H

#include "libel/net/callbacks.h"
#include "libel/net/tcp_connection.h"

#include <atomic>
#include <map>

namespace Libel {

namespace net {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// Tcp server, supports single-threaded and thread-pool models.
///
class TcpServer : noncopyable {
 public:
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  enum Option {
    kNoReusePort,
    kReusePort,
  };

  TcpServer(EventLoop* loop, const InetAddress& listenAddr,
            std::string nameArg, Option option = kNoReusePort);
  ~TcpServer(); // force out-line dtor, for std::unique_ptr members

  const std::string &ipPort() const { return ipPort_; }
  const std::string &name() const { return name_; }
  EventLoop* getLoop() const { return loop_; }

  /// Set the number of threads for handling output.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @func start
  /// @param numTheads
  /// - 0 means all I/O in loop's thread, no thread will be created.
  /// this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N thread, new connections
  /// are all assigned on a round-robin basis.
  void setThreadNum(int numTheads);
  void setThreadInitCallback(ThreadInitCallback cb) {
    threadInitCallback_ = std::move(cb);
  }

  /// valid after calling start()
  std::shared_ptr<EventLoopThreadPool> threadPool() {
    return threadPool_;
  }

  /// Starts the server if it's not listening.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();

  /// Set connection callback
  /// Not thread safe
  void setConnectionCallback(ConnectionCallback cb) {
    connectionCallback_ = std::move(cb);
  }

  /// Set message callback
  void setMessageCallback(MessageCallback cb) {
    messageCallback_ = std::move(cb);
  }

  void setWriteCompleteCallback(WriteCompleteCallback cb) {
    writeCompleteCallback_ = std::move(cb);
  }

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress& peerAddr);
  /// Thread safe.
  void removeConnection(const TcpConnectionPtr& conn);
  /// Not thread safe, but in loop
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

  using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
  EventLoop* loop_;  // the acceptor loop
  const std::string ipPort_;
  const std::string name_;
  std::unique_ptr<Acceptor> acceptor_;
  std::shared_ptr<EventLoopThreadPool> threadPool_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  ThreadInitCallback threadInitCallback_;
  std::atomic_flag started_;
  int nextConnId_;  // always in loop thread, so no need to be atomic
  ConnectionMap connections_;
};

}  // namespace net
}  // namespace Libel

#endif  // LIBEL_TCP_SERVER_H
