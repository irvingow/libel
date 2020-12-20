//
// Created by kaymind on 2020/12/20.
//

#include "libel/net/http/http_context.h"
#include "libel/net/buffer.h"

using namespace Libel;
using namespace Libel::net;

bool HttpContext::processRequestLine(const char* begin, const char* end) {
  bool success = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  if (space != end && request_.setMethod(start, space)) {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end) {
      const char* question = std::find(start, space, ' ');
      if (question != space) {
        request_.setPath(start, question);
        request_.setQuery(question, space);
      } else {
        request_.setPath(start, space);
      }
      start = space + 1;
      success = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
      if (success) {
        if (*(end - 1) == '1')
          request_.setVersion(HttpRequest::kHttp11);
        else if (*(end - 1) == '0')
          request_.setVersion(HttpRequest::kHttp10);
        else
          success = false;
      }
    }
  }
  return success;
}

bool HttpContext::parseRequest(Buffer* buffer, TimeStamp receiveTime) {
  bool ok = true, hasMore = true;
  while (hasMore) {
    if (state_ == kExpectRequestLine) {
      auto crlf = buffer->findCRLF();
      if (crlf) {
        ok = processRequestLine(buffer->peek(), crlf);
        if (ok) {
          request_.setReceiveTime(receiveTime);
          buffer->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        } else {
          hasMore = false;
        }
      } else {
        hasMore = false;
      }
    } else if (state_ == kExpectHeaders) {
      auto crlf = buffer->findCRLF();
      if (crlf) {
        const char* colon = std::find(buffer->peek(), crlf, ':');
        if (colon != crlf) {
          request_.addHeader(buffer->peek(), colon, crlf);
        } else {
          // empty line, end of header
          state_ = kGotAll;
          hasMore = false;
        }
        buffer->retrieveUntil(crlf + 2);
      } else {
        hasMore = false;
      }
    } else if (state_ == kExpectBody) {
      // FIXME
      break;
    }
  }
  return ok;
}