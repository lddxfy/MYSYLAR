#include "../include/hash_util.h"
#include <openssl/sha.h>

namespace lamb{

std::string base64decode(const std::string &src){
    std::string res;
    //Base64编码的每4个字符代表3个字节的数据
    res.resize(sizeof(src) * 3 / 4);
    //获取result字符串的首地址
    char *writeBuf = &res[0];

    const char *ptr = src.c_str();
    const char *end = ptr + src.size();
    while(ptr < end){
        //用于计数当前处理的字符在Base64字符组中的位置。
        int i = 0;
        //用于记录输入字符串中的填充字符=的数量。
        int padding = 0;
        //用于存储当前处理的4个Base64字符转换后的值。
        int packed = 0;
        for(;i < 4 && ptr < end;i++,ptr++){
            if (*ptr == '=') {
                ++padding;
                packed <<= 6;
                continue;
            }

            // padding with "=" only
            if (padding > 0) {
                return "";
            }

            int val = 0;
            if (*ptr >= 'A' && *ptr <= 'Z') {
                val = *ptr - 'A';
            } else if (*ptr >= 'a' && *ptr <= 'z') {
                val = *ptr - 'a' + 26;
            } else if (*ptr >= '0' && *ptr <= '9') {
                val = *ptr - '0' + 52;
            } else if (*ptr == '+') {
                val = 62;
            } else if (*ptr == '/') {
                val = 63;
            } else {
                return ""; // invalid character
            }

            packed = (packed << 6) | val;
        }
        if (i != 4) {
            return "";
        }
        if (padding > 0 && ptr != end) {
            return "";
        }
        if (padding > 2) {
            return "";
        }

        *writeBuf++ = (char)((packed >> 16) & 0xff);
        if (padding != 2) {
            *writeBuf++ = (char)((packed >> 8) & 0xff);
        }
        if (padding == 0) {
            *writeBuf++ = (char)(packed & 0xff);
        }
    }

    res.resize(writeBuf-res.c_str());
    return res;
}

std::string base64encode(const std::string &data) {
    return base64encode(data.c_str(), data.size());
}

std::string base64encode(const void *data, size_t len) {
    const char *base64 =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string ret;
    ret.reserve(len * 4 / 3 + 2);

    const unsigned char *ptr = (const unsigned char *)data;
    const unsigned char *end = ptr + len;

    while (ptr < end) {
        unsigned int packed = 0;
        int i               = 0;
        int padding         = 0;
        for (; i < 3 && ptr < end; ++i, ++ptr) {
            packed = (packed << 8) | *ptr;
        }
        if (i == 2) {
            padding = 1;
        } else if (i == 1) {
            padding = 2;
        }
        for (; i < 3; ++i) {
            packed <<= 8;
        }

        ret.append(1, base64[packed >> 18]);
        ret.append(1, base64[(packed >> 12) & 0x3f]);
        if (padding != 2) {
            ret.append(1, base64[(packed >> 6) & 0x3f]);
        }
        if (padding == 0) {
            ret.append(1, base64[packed & 0x3f]);
        }
        ret.append(padding, '=');
    }

    return ret;
}

std::string sha1sum(const std::string &data){
    return sha1sum(data.c_str(), data.size());
}

std::string sha1sum(const void *data, size_t len){
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, len);
    std::string result;
    result.resize(SHA_DIGEST_LENGTH);
    SHA1_Final((unsigned char *)&result[0], &ctx);
    return result;
}

std::string random_string(size_t len, const std::string &chars){
    if(len == 0 || chars.empty()){
        return "";
    }
    std::string rt;
    rt.resize(len);
    int count = chars.size();
    for(size_t i = 0;i < len;++i){
        rt[i] = chars[rand() % count];
    }
    return rt;
}


}