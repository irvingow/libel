//
// Created by kaymind on 2021/1/10.
//

#include "libel/net/protorpc/RpcCodec.h"

#include "libel/base/logging.h"
#include "libel/net/Endian.h"
#include "libel/net/tcp_connection.h"

#include "libel/net/protorpc/rpc.pb.h"
#include "libel/net/protorpc/google-inl.h"

using namespace Libel;
using namespace Libel::net;

namespace {

int ProtobufVersionCheck() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  return 0;
}
int __attribute__ ((unused)) dummy = ProtobufVersionCheck();

}

namespace Libel {

namespace net {
const char rpctag[] = "RPC0";
}
}