//
// Created by kaymind on 2020/12/20.
//

#ifndef LIBEL_HTTP_CONTEXT_H
#define LIBEL_HTTP_CONTEXT_H

#include "libel/net/http/http_request.h"
#include <memory>

namespace Libel {

namespace net {

class Buffer;

class HttpContext {
public:
  enum HttpRequestParseState {
    kExpectRequestLine,
    kExpectHeaders,
    kExpectBody,
    kGotAll,
  };

  HttpContext() : state_(kExpectRequestLine) {}

  // defautl copy-ctor, dtor and assignement are just fine.

  /// return false is any error
  bool parseRequest(Buffer* buffer, TimeStamp receiveTime);

  bool gotAll() const {
    return state_ == kGotAll;
  }

  void reset() {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest& getRequest() const {
    return request_;
  }

  HttpRequest& getRequest() {
    return request_;
  }

private:
  bool processRequestLine(const char* begin, const char* end);

  HttpRequestParseState state_;
  HttpRequest request_;
};

}
}

#endif //LIBEL_HTTP_CONTEXT_H
