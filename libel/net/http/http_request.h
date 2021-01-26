//
// Created by kaymind on 2020/12/20.
//

#ifndef LIBEL_HTTP_REQUEST_H
#define LIBEL_HTTP_REQUEST_H

#include "libel/base/timestamp.h"
#include "libel/net/callbacks.h"

#include <cassert>
#include <cstdio>
#include <map>

namespace Libel {

namespace net {

class HttpRequest {
 public:
  enum Method { kInvalid, kGet, kPost, kHead, kPut, kDelete };
  enum Version { kUnknown, kHttp10, kHttp11 };

  HttpRequest() : method_(kInvalid), version_(kUnknown) {}

  void setVersion(Version version) { version_ = version; }

  Version getVersion() const { return version_; }

  bool setMethod(const char* start, const char* end) {
    assert(method_ == kInvalid);
    std::string method(start, end);
    if (method == "GET") {
      method_ = kGet;
    } else if (method == "POST") {
      method_ = kPost;
    } else if (method == "HEAD") {
      method_ = kHead;
    } else if (method == "PUT") {
      method_ = kPut;
    } else if (method == "DELETE") {
      method_ = kDelete;
    } else {
      method_ = kInvalid;
    }
    return method_ != kInvalid;
  }

  Method getMethod() const { return method_; }

  const char* methodString() const {
    const char* result = "UNKNOWN";
    switch (method_) {
      case kGet:
        result = "GET";
        break;
      case kPost:
        result = "POST";
        break;
      case kHead:
        result = "HEAD";
        break;
      case kPut:
        result = "PUT";
        break;
      case kDelete:
        result = "DELETE";
        break;
      default:
        break;
    }
    return result;
  }

  void setPath(const char* start, const char* end) { path_.assign(start, end); }

  const std::string& getPath() const { return path_; }

  void setQuery(const char* start, const char* end) {
    query_.assign(start, end);
  }

  const std::string& query() const { return query_; }

  void setReceiveTime(TimeStamp timeStamp) { receiveTime_ = timeStamp; }

  TimeStamp getReceiveTime() const { return receiveTime_; }

  void addHeader(const char* start, const char* colon, const char* end) {
    std::string field(start, colon);
    ++colon;
    while (colon < end && isspace(*colon)) ++colon;
    std::string value(colon, end);
    while (!value.empty() && isspace(value.back())) {
      value.resize(value.size() - 1);
    }
    headers_[field] = value;
  }

  std::string getHeader(const std::string& field) const {
    std::string result;
    const auto iter = headers_.find(field);
    if (iter != headers_.end()) result = iter->second;
    return result;
  }

  const std::map<std::string, std::string>& headers() const { return headers_; }

  void swap(HttpRequest& that) {
    std::swap(method_, that.method_);
    std::swap(version_, that.version_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    receiveTime_.swap(that.receiveTime_);
    headers_.swap(that.headers_);
  }

 private:
  Method method_;
  Version version_;
  std::string path_;
  std::string query_;
  TimeStamp receiveTime_;
  std::map<std::string, std::string> headers_;
};

}  // namespace net
}  // namespace Libel

#endif  // LIBEL_HTTP_REQUEST_H
