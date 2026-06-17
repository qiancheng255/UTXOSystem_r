/**
 * RSA加密模块实现文件
 * 使用简化的RSA实现（用于演示）
 * 生产环境应使用完整的RSA库如OpenSSL
 */

#include "rsa_crypto.h"
#include "crypto.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <ctime>
#include <algorithm>
#include <random>

namespace RSACrypto {

    // Base64字符表
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string base64_encode(const std::vector<unsigned char>& data) {
        std::string result;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        int in_len = static_cast<int>(data.size());
        const unsigned char* bytes_to_encode = data.data();

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (i = 0; i < 4; i++)
                    result += base64_chars[char_array_4[i]];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 3; j++)
                char_array_3[j] = '\0';

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

            for (j = 0; j < i + 1; j++)
                result += base64_chars[char_array_4[j]];

            while (i++ < 3)
                result += '=';
        }

        return result;
    }

    std::vector<unsigned char> base64_decode(const std::string& encoded) {
        int in_len = static_cast<int>(encoded.size());
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::vector<unsigned char> result;

        while (in_len-- && (encoded[in_] != '=') && 
               (isalnum(encoded[in_]) || (encoded[in_] == '+') || (encoded[in_] == '/'))) {
            char_array_4[i++] = encoded[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; i < 3; i++)
                    result.push_back(char_array_3[i]);
                i = 0;
            }
        }

        if (i) {
            for (j = 0; j < i; j++)
                char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

            for (j = 0; j < i - 1; j++)
                result.push_back(char_array_3[j]);
        }

        return result;
    }

    // 简化的RSA密钥对生成（基于SHA256哈希）
    RSAKeyPair generate_key_pair(int key_size) {
        RSAKeyPair key_pair;
        
        // 生成随机种子
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        // 生成私钥种子
        std::string private_seed;
        for (int i = 0; i < 32; i++) {
            private_seed += static_cast<char>(dis(gen));
        }
        
        // 生成公钥种子
        std::string public_seed;
        for (int i = 0; i < 32; i++) {
            public_seed += static_cast<char>(dis(gen));
        }
        
        // 使用SHA256生成密钥
        key_pair.private_key = Crypto::sha256(private_seed + "_rsa_private_key");
        key_pair.public_key = Crypto::sha256(public_seed + "_rsa_public_key");
        
        // 生成密钥ID
        key_pair.key_id = Crypto::sha256(key_pair.public_key).substr(0, 16);
        
        return key_pair;
    }

    // 使用私钥对数据进行签名
    DigitalSignature sign_data(const std::string& private_key, 
                               const std::string& data,
                               const std::string& signer_id) {
        DigitalSignature sig;
        sig.signer_id = signer_id;
        
        // 计算数据哈希
        sig.data_hash = Crypto::sha256(data);
        
        // 获取时间戳
        time_t now = time(nullptr);
        char time_str[26];
        ctime_s(time_str, sizeof(time_str), &now);
        sig.timestamp = time_str;
        
        // 使用私钥和数据哈希生成签名
        std::string sign_input = sig.data_hash + private_key + sig.timestamp;
        sig.signature = Crypto::sha256(sign_input);
        
        return sig;
    }

    // 验证数字签名
    bool verify_signature(const std::string& public_key,
                          const std::string& data,
                          const DigitalSignature& signature) {
        // 首先验证数据哈希
        std::string computed_hash = Crypto::sha256(data);
        if (computed_hash != signature.data_hash) {
            return false;
        }
        
        // 验证签名
        // 注意：这是一个简化的验证，实际RSA验证需要使用公钥解密签名并与哈希比较
        // 这里我们重新计算签名并比较
        std::string expected_input = signature.data_hash + public_key + signature.timestamp;
        std::string expected_signature = Crypto::sha256(expected_input);
        
        return expected_signature == signature.signature;
    }

    // 使用公钥加密数据
    std::string encrypt_with_public_key(const std::string& public_key,
                                        const std::string& data) {
        // 使用简化的方式模拟RSA加密
        std::string encrypted = Crypto::sha256(public_key + data);
        return base64_encode(std::vector<unsigned char>(encrypted.begin(), encrypted.end()));
    }

    // 使用私钥解密数据
    std::string decrypt_with_private_key(const std::string& private_key,
                                         const std::string& encrypted_data) {
        // 简化的解密（实际应用中需要真正的RSA解密）
        return encrypted_data;
    }

    // 获取公钥指纹
    std::string get_public_key_fingerprint(const std::string& public_key) {
        return Crypto::sha256(public_key).substr(0, 32);
    }

    // 验证密钥对是否匹配
    bool verify_key_pair(const std::string& public_key, const std::string& private_key) {
        // 通过签名和验证来检查密钥对是否匹配
        std::string test_data = "test_data_for_verification";
        DigitalSignature sig = sign_data(private_key, test_data, "test");
        return verify_signature(public_key, test_data, sig);
    }

} // namespace RSACrypto