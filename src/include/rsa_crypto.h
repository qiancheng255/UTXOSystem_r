/**
 * RSA加密模块头文件
 * 实现RSA数字签名用于交易认证
 */

#ifndef RSA_CRYPTO_H
#define RSA_CRYPTO_H

#include <string>
#include <vector>
#include <memory>

namespace RSACrypto {

    // RSA密钥对结构
    struct RSAKeyPair {
        std::string public_key;   // 公钥（PEM格式）
        std::string private_key;  // 私钥（PEM格式）
        std::string key_id;       // 密钥标识
    };

    // 数字签名结构
    struct DigitalSignature {
        std::string signature;    // 签名数据（Base64编码）
        std::string signer_id;    // 签名者标识
        std::string timestamp;    // 签名时间戳
        std::string data_hash;    // 原始数据哈希
    };

    /**
     * 生成RSA密钥对
     * @param key_size 密钥长度（推荐2048位）
     * @return RSA密钥对
     */
    RSAKeyPair generate_key_pair(int key_size = 2048);

    /**
     * 使用私钥对数据进行签名
     * @param private_key 私钥（PEM格式）
     * @param data 要签名的数据
     * @return 数字签名
     */
    DigitalSignature sign_data(const std::string& private_key, 
                               const std::string& data,
                               const std::string& signer_id);

    /**
     * 使用公钥验证数字签名
     * @param public_key 公钥（PEM格式）
     * @param data 原始数据
     * @param signature 数字签名
     * @return 签名是否有效
     */
    bool verify_signature(const std::string& public_key,
                          const std::string& data,
                          const DigitalSignature& signature);

    /**
     * 使用公钥加密数据
     * @param public_key 公钥（PEM格式）
     * @param data 要加密的数据
     * @return 加密后的数据（Base64编码）
     */
    std::string encrypt_with_public_key(const std::string& public_key,
                                        const std::string& data);

    /**
     * 使用私钥解密数据
     * @param private_key 私钥（PEM格式）
     * @param encrypted_data 加密的数据（Base64编码）
     * @return 解密后的数据
     */
    std::string decrypt_with_private_key(const std::string& private_key,
                                         const std::string& encrypted_data);

    /**
     * 从密钥对中提取公钥指纹
     * @param public_key 公钥（PEM格式）
     * @return 公钥指纹（用于标识密钥）
     */
    std::string get_public_key_fingerprint(const std::string& public_key);

    /**
     * 验证密钥对是否匹配
     * @param public_key 公钥
     * @param private_key 私钥
     * @return 是否匹配
     */
    bool verify_key_pair(const std::string& public_key, const std::string& private_key);

    /**
     * Base64编码
     */
    std::string base64_encode(const std::vector<unsigned char>& data);

    /**
     * Base64解码
     */
    std::vector<unsigned char> base64_decode(const std::string& encoded);

} // namespace RSACrypto

#endif // RSA_CRYPTO_H