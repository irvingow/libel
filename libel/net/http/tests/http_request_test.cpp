//
// Created by kaymind on 2020/12/20.
//

#include "libel/net/http/http_context.h"
#include "libel/net/buffer.h"

using namespace Libel;
using namespace Libel::net;

void testParseRequestAllInOne() {
  HttpContext context;
  Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
               "Host: www.lwj.com\r\n"
               "\r\n");
  assert(context.parseRequest(&input, TimeStamp::now()));
  assert(context.gotAll());
  const auto& request = context.getRequest();
  assert(request.getMethod() == HttpRequest::kGet);
  assert(request.getPath() == "/index.html");
  assert(request.getVersion() == HttpRequest::kHttp11);
  assert(request.getHeader("Host") == std::string("www.lwj.com"));
  assert(request.getHeader("User-Agent") == std::string(""));
}

void testParseRequestInTwoPieces() {
  std::string all("GET /index.html HTTP/1.1\r\n"
             "Host: www.lwj.com\r\n"
             "\r\n");
  for (size_t len = 0; len < all.size(); ++len) {
    HttpContext context;
    Buffer input;
    // notice that len is always less than all.size(),
    // so contents are always not enough
    input.append(all.c_str(), len);
    assert(context.parseRequest(&input, TimeStamp::now()));
    assert(!context.gotAll());

    auto add = all.size() - len;
    input.append(all.c_str() + len, add);
    assert(context.parseRequest(&input, TimeStamp::now()));
    assert(context.gotAll());
    const HttpRequest& request = context.getRequest();
    assert(request.getMethod() == HttpRequest::kGet);
    assert(request.getPath() == "/index.html");
    assert(request.getVersion() == HttpRequest::kHttp11);
    assert(request.getHeader("Host") == std::string("www.lwj.com"));
    assert(request.getHeader("User-Agent") == std::string(""));
  }
}

void testParseRequestEmptyHeaderValue() {
  HttpContext context;
  Buffer input;
  input.append("GET /index.html HTTP/1.1\r\n"
               "Host: www.lwj.com\r\n"
               "User-Agent:\r\n"
               "Accept-Encoding: \r\n"
               "\r\n");

  assert(context.parseRequest(&input, TimeStamp::now()));
  assert(context.gotAll());
  const auto& request = context.getRequest();
  assert(request.getMethod() == HttpRequest::kGet);
  assert(request.getPath() == "/index.html");
  assert(request.getVersion() == HttpRequest::kHttp11);
  assert(request.getHeader("Host") == std::string("www.lwj.com"));
  assert(request.getHeader("User-Agent") == std::string(""));
  assert(request.getHeader("Accept-Encoding") == std::string(""));
}

int main() {
  return 0;
}