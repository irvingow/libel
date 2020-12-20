//
// Created by kaymind on 2020/12/20.
//

#include "libel/net/buffer.h"
#include "libel/net/http/http_response.h"

#include <cstdio>

using namespace Libel;
using namespace Libel::net;

void HttpResponse::appendToBuffer(Buffer* outputBuffer) const {
  char buf[32] = {};
  snprintf(buf, sizeof buf, "HTTP/1.1 %d", statusCode_);
  outputBuffer->append(buf);
  outputBuffer->append(statusMessage_);
  outputBuffer->append("\r\n");
  if (closeConnection_)
    outputBuffer->append("Connection: close\r\n");
  else {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    outputBuffer->append(buf);
    outputBuffer->append("Connection: Keep-Alive\r\n");
  }
  for (const auto& header : headers_) {
    outputBuffer->append(header.first);
    outputBuffer->append(": ");
    outputBuffer->append(header.second);
    outputBuffer->append("\r\n");
  }
  outputBuffer->append("\r\n");
  outputBuffer->append(body_);
}
