//
// Created by kaymind on 2020/12/17.
//

#include "libel/base/logging.h"
#include "libel/net/inet_address.h"

using namespace Libel;
using namespace Libel::net;

void testInetAddress() {
  InetAddress address(1234);
  assert(address.toIpPort() == std::string("0.0.0.0:1234"));
  assert(address.toIp() == std::string("0.0.0.0"));
  assert(address.toPort() == 1234);

  InetAddress address1(1111, true);
  assert(address1.toPort() == 1111);
  assert(address1.toIp() == std::string("127.0.0.1"));
  assert(address1.toIpPort() == std::string("127.0.0.1:1111"));

  InetAddress address2("192.168.1.1", 9999);
  assert(address2.toPort() == 9999);
  assert(address2.toIp() == std::string("192.168.1.1"));
  assert(address2.toIpPort() == std::string("192.168.1.1:9999"));

  InetAddress address3("255.254.253.252", 65535);
  assert(address3.toPort() == 65535);
  assert(address3.toIp() == std::string("255.254.253.252"));
  assert(address3.toIpPort() == std::string("255.254.253.252:65535"));
}

void testInetAddressResolve() {
  InetAddress address(80);
  if (InetAddress::resolve("baidu.com", &address)) {
    LOG_INFO << "baidu.com resolved to " << address.toIpPort();
  } else {
    LOG_ERROR << "Unable to resolve baidu.com";
  }
}

int main() {
  testInetAddress();
  testInetAddressResolve();
}
