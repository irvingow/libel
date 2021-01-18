//
// Created by kaymind on 2021/1/13.
//

#include "examples/protobuf/codec/codec.h"

#include "libel/base/logging.h"
#include "libel/net/Endian.h"
#include "libel/net/protorpc/google-inl.h"

#include <google/protobuf/descriptor.h>

#include <zlib.h> // adler32

using namespace Libel;
using namespace Libel::net;

