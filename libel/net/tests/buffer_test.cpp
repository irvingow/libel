//
// Created by kaymind on 2020/12/11.
//

#include "libel/net/buffer.h"
#include <iostream>

using namespace Libel;
using namespace Libel::net;

void testBufferAppendRetrieve() {
  Buffer buffer;
  assert(buffer.readableBytes() == 0);
  assert(buffer.writableBytes() == Buffer::kInitialSize);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend);

  const std::string str(200, 'x');
  buffer.append(str);
  assert(buffer.readableBytes() == str.size());
  assert(buffer.writableBytes() == Buffer::kInitialSize - str.size());
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend);

  const std::string str2 = buffer.retrieveAsString(50);
  assert(str2.size() == 50);
  assert(buffer.readableBytes() == str.size() - str2.size());
  assert(buffer.writableBytes() == Buffer::kInitialSize - str.size());
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend + str2.size());
  assert(str2 == str.substr(0, 50));

  buffer.append(str);
  assert(buffer.readableBytes() == 2 * str.size() - str2.size());
  assert(buffer.writableBytes() == Buffer::kInitialSize - 2 * str.size());
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend + str2.size());

  std::string str3 = buffer.retrieveAllAsString();
  assert(str3.size() == 350);
  assert(buffer.readableBytes() == 0);
  assert(buffer.writableBytes() == Buffer::kInitialSize);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend);
  assert(str3 == std::string(350, 'x'));
}

void testBufferGrow() {
  Buffer buffer;
  buffer.append(std::string(400, 'x'));
  assert(buffer.readableBytes() == 400);
  assert(buffer.writableBytes() == Buffer::kInitialSize - 400);

  buffer.retrieve(50);
  assert(buffer.readableBytes() == 350);
  assert(buffer.writableBytes() == Buffer::kInitialSize - 400);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend + 50);

  buffer.append(std::string(1000, 'y'));
  assert(buffer.readableBytes() == 1350);
  assert(buffer.writableBytes() == 0);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend + 50);

  buffer.retrieveAll();
  assert(buffer.readableBytes() == 0);
  assert(buffer.writableBytes() == 1400);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend);
}

void testBufferInsideGrow() {
  Buffer buffer;
  buffer.append(std::string(800, 'x'));
  assert(buffer.readableBytes() == 800);
  assert(buffer.writableBytes() == Buffer::kInitialSize - 800);

  buffer.retrieve(500);
  assert(buffer.readableBytes() == 300);
  assert(buffer.writableBytes() == Buffer::kInitialSize - 800);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend + 500);

  buffer.append(std::string(300, 'y'));
  assert(buffer.readableBytes() == 600);
  assert(buffer.writableBytes() == Buffer::kInitialSize - 600);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend);
}

void testBufferShrink() {
  Buffer buffer;
  buffer.append(std::string(2000, 'y'));
  assert(buffer.readableBytes() == 2000);
  assert(buffer.writableBytes() == 0);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend);

  buffer.retrieve(1500);
  assert(buffer.readableBytes() == 500);
  assert(buffer.writableBytes() == 0);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend + 1500);

  buffer.shrink(0);
  assert(buffer.readableBytes() == 500);
  assert(buffer.writableBytes() == Buffer::kInitialSize - 500);
  assert(buffer.retrieveAllAsString() == std::string(500, 'y'));
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend);
}

void testBufferPrepend() {
  Buffer buffer;
  buffer.append(std::string(200, 'x'));
  assert(buffer.readableBytes() == 200);
  assert(buffer.writableBytes() == Buffer::kInitialSize - 200);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend);

  int x = 0;
  buffer.prepend(&x, sizeof(x));
  assert(buffer.readableBytes() == 204);
  assert(buffer.writableBytes() == Buffer::kInitialSize - 200);
  assert(buffer.prependableBytes() == Buffer::kCheapPrepend - 4);
}

void testBufferReadInt() {
  Buffer buffer;
  buffer.append("HTTP");

  assert(buffer.readableBytes() == 4);
  assert(buffer.peekInt8() == 'H');
  int top16 = buffer.peekInt16();
  (void)top16;
  assert(top16 == 'H' * 256 + 'T');
  assert(buffer.peekInt32() == top16 * 65536 + 'T' * 256 + 'P');

  assert(buffer.readInt8() == 'H');
  assert(buffer.readInt16() == 'T' * 256 + 'T');
  assert(buffer.readInt8() == 'P');
  assert(buffer.readableBytes() == 0);
  assert(buffer.writableBytes() == Buffer::kInitialSize);

  buffer.appendInt8(-1);
  buffer.appendInt16(-2);
  buffer.appendInt32(-3);
  assert(buffer.readableBytes() == 7);
  assert(buffer.readInt8() == -1);
  assert(buffer.readInt16() == -2);
  assert(buffer.readInt32() == -3);
}

void testBufferFindEOL() {
  Buffer buffer;
  buffer.append(std::string(100000, 'x'));
  assert(buffer.findEOL() == nullptr);
  assert(buffer.findEOL(buffer.peek() + 90000) == nullptr);
}

void output(Buffer&& buffer, const void* inner) {
  Buffer newBuffer(std::move(buffer));
  assert(inner == newBuffer.peek());
}

void testMove() {
  Buffer buffer;
  buffer.append("libel", 5);
  const void* inner = buffer.peek();
  output(std::move(buffer), inner);
}

int main() {
  testMove();
  testBufferAppendRetrieve();
  testBufferGrow();
  testBufferInsideGrow();
  testBufferShrink();
  testBufferPrepend();
  testBufferReadInt();
  testBufferFindEOL();
  return 0;
}
