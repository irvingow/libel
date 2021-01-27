//
// Created by kaymind on 2020/12/20.
//

#ifndef LIBEL_HTTP_RESPONSE_H
#define LIBEL_HTTP_RESPONSE_H

#include "libel/net/callbacks.h"

#include <map>
#include <memory>

namespace Libel {

namespace net {

class Buffer;
class HttpResponse {
public:
  enum HttpStatusCode {
    kUnknown,
    k2000k = 200,
    k301MovePermanently = 400,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  explicit HttpResponse(bool close) : statusCode_(kUnknown), closeConnection_(close) {

  }

  // default copy-ctor, dtor and assignment are just fine.

  void setStatusCode(HttpStatusCode code) {
    statusCode_ = code;
  }

  void setStatusMessage(std::string message) {
    statusMessage_ = std::move(message);
  }

  void setCloseConnection(bool on) {
    closeConnection_ = on;
  }

  bool closeConnection() const {
    return closeConnection_;
  }

  void setContentType(const std::string& contentType) {
    addHeader("Content-type", contentType);
  }

  void setContentLength(const std::string& contentLength) {
    addHeader("Content-Length", contentLength);
  }

  void addHeader(const std::string& key, const std::string &value) {
    headers_[key] = value;
  }

  void setBody(std::string body) {
    body_ = std::move(body);
  }

  void appendToBuffer(Buffer* buffer) const;

  void clear();

private:
  std::map<std::string, std::string> headers_;
  HttpStatusCode statusCode_;
  // FIXME: add http version
  std::string statusMessage_;
  bool closeConnection_;
  std::string body_;
};

}
}

#endif //LIBEL_HTTP_RESPONSE_H
