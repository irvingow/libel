//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/rpc/sudoku.pb.h"

#include "libel/base/logging.h"
#include "libel/net/eventloop.h"
#include "libel/net/protorpc/RpcServer.h"

#include <unistd.h>

using namespace Libel;
using namespace Libel::net;

namespace sudoku {

class SudokuServiceImpl : public SudokuService {
public:
  virtual void Solve(::google::protobuf::RpcController* controller,
                     const ::sudoku::SudokuRequest* request,
                     ::sudoku::SudokuResponse* response,
                     ::google::protobuf::Closure* done) override {
    LOG_INFO << "SudokuServiceImpl::Solve";
    response->set_solved(true);
    response->set_checkboard("12345678");
    done->Run();
  }
};

}

int main() {
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(9981);
  sudoku::SudokuServiceImpl impl;
  RpcServer server(&loop, listenAddr);
  server.registerService(&impl);
  server.start();
  loop.loop();
  google::protobuf::ShutdownProtobufLibrary();
}