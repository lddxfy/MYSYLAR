#ifndef __SYLAR_UTIL_HASH_UTIL_H__
#define __SYLAR_UTIL_HASH_UTIL_H__

#include <string>

namespace sylar{
//将Base64编码的字符串解码成原始的字节序列
std::string base64decode(const std::string &src);
//将字符串编码为Base64编码
std::string base64encode(const std::string &data);
//将字符串编码为Base64编码
std::string base64encode(const void *data, size_t len);
//计算数据的SHA-1哈希值
std::string sha1sum(const std::string &data);\
//计算数据的SHA-1哈希值
std::string sha1sum(const void *data, size_t len);

std::string random_string(size_t len, const std::string &chars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

}


#endif