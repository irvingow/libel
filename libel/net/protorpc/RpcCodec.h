//
// Created by kaymind on 2021/1/10.
//

#ifndef LIBEL_RPCCODEC_H
#define LIBEL_RPCCODEC_H

#include "libel/base/timestamp.h"
#include "libel/net/protobuf/ProtobufCodecLite.h"

namespace Libel {

namespace net {

class Buffer;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class RpcMessage;
using RpcMessagePtr = std::shared_ptr<RpcMessage>;
extern const char rpctag[]; // = "RPC0"

using RpcCodec = ProtobufCodecLiteT<RpcMessage, rpctag>;

}
}

#endif //LIBEL_RPCCODEC_H
