// test_endian.cpp
#include "endian.h"
#include <cstdint>



int main() {
    // 使用 endian.h 中的一个函数，例如 byteswapOnLittleEndian，以确保头文件包含成功
    uint32_t testValue = 0x12345678;
    uint32_t swappedValue = sylar::byteswapOnLittleEndian(testValue);
    return 0;
}