//
// Created by kaymind on 2021/1/27.
//

#ifndef LIBEL_WEBSERVER_H
#define LIBEL_WEBSERVER_H

#include "libel/base/noncopyable.h"

#include <string>
#include <unordered_map>

class MimeType : Libel::noncopyable {
 public:
  static std::string getMime(const std::string& suffix);

 private:
  static void init();
  static std::unordered_map<std::string, std::string> mime_;

  static pthread_once_t once_control_;
};

#endif  // LIBEL_WEBSERVER_H
