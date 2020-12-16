//
// Created by 刘文景 on 2020-03-25.
//

#include "libel/base/num2string.h"
#include <string>
#include <cassert>

using namespace Libel::Util;

int main(){
    uint32_t u32 = UINT32_MAX;
    int32_t i32 = INT32_MIN;
    uint64_t u64 = UINT64_MAX;
    int64_t i64 = INT64_MAX;
    uint16_t u16 = UINT16_MAX;
    int16_t i16 = INT16_MIN;
    char buf[128] = {0};
    u32toa(u32, buf);
    std::string u32_str(buf);
    assert(u32_str == std::to_string(u32));
    i32toa(i32, buf);
    std::string i32_str(buf);
    assert(i32_str == std::to_string(i32));
    u64toa(u64, buf);
    std::string u64_str(buf);
    assert(u64_str == std::to_string(u64));
    i64toa(i64, buf);
    std::string i64_str(buf);
    assert(i64_str == std::to_string(i64));
    u16toa(u16, buf);
    std::string u16_str(buf);
    assert(u16_str == std::to_string(u16));
    i16toa(i16, buf);
    std::string i16_str(buf);
    assert(i16_str == std::to_string(i16));
}
