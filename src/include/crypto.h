/**
 * 加密模块头文件
 * 使用Windows BCrypt API实现生产环境加密
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>

namespace Crypto {
    std::string sha256(const std::string& input);
    std::string generate_private_key(const std::string& address);
    std::string sign_transaction(const std::string& private_key, const std::string& data);
    bool verify_signature(const std::string& public_key, 
                         const std::string& data, 
                         const std::string& signature);
}

#endif // CRYPTO_H