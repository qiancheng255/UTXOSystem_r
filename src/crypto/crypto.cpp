/**
 * 加密模块实现文件
 * 使用Windows BCrypt API实现生产环境加密
 */

#include "crypto.h"
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

// Windows加密API头文件
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>

// 链接Windows加密库
#pragma comment(lib, "bcrypt.lib")

namespace Crypto {
    
    std::string sha256(const std::string& input) {
        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_HASH_HANDLE hHash = NULL;
        NTSTATUS status;
        std::string result;
        
        try {
            status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法打开SHA256算法提供者");
            }
            
            DWORD hashSize = 0;
            DWORD dataSize = 0;
            status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&hashSize, sizeof(DWORD), &dataSize, 0);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法获取哈希大小");
            }
            
            status = BCryptCreateHash(hAlg, &hHash, NULL, 0, NULL, 0, 0);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法创建哈希对象");
            }
            
            status = BCryptHashData(hHash, (PBYTE)input.c_str(), (ULONG)input.length(), 0);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法添加数据到哈希");
            }
            
            std::vector<BYTE> hashBuffer(hashSize);
            status = BCryptFinishHash(hHash, hashBuffer.data(), hashSize, 0);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法完成哈希计算");
            }
            
            std::stringstream ss;
            for (DWORD i = 0; i < hashSize; i++) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hashBuffer[i]);
            }
            result = ss.str();
            
        } catch (...) {
            if (hHash) BCryptDestroyHash(hHash);
            if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
            throw;
        }
        
        if (hHash) BCryptDestroyHash(hHash);
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
        
        return result;
    }
    
    std::string generate_private_key(const std::string& address) {
        unsigned char random_bytes[32];
        NTSTATUS status = BCryptGenRandom(NULL, random_bytes, sizeof(random_bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (!BCRYPT_SUCCESS(status)) {
            throw std::runtime_error("随机数生成失败");
        }
        
        std::string seed = "private_key_" + address;
        seed.append(reinterpret_cast<char*>(random_bytes), sizeof(random_bytes));
        
        return sha256(seed);
    }
    
    std::string sign_transaction(const std::string& private_key, const std::string& data) {
        BCRYPT_ALG_HANDLE hAlg = NULL;
        BCRYPT_HASH_HANDLE hHash = NULL;
        NTSTATUS status;
        std::string result;
        
        try {
            status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法打开HMAC-SHA256算法提供者");
            }
            
            status = BCryptCreateHash(hAlg, &hHash, NULL, 0, 
                                     (PBYTE)private_key.c_str(), (ULONG)private_key.length(), 0);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法创建HMAC哈希对象");
            }
            
            status = BCryptHashData(hHash, (PBYTE)data.c_str(), (ULONG)data.length(), 0);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法添加数据到HMAC");
            }
            
            DWORD hashSize = 32;
            std::vector<BYTE> hmacBuffer(hashSize);
            status = BCryptFinishHash(hHash, hmacBuffer.data(), hashSize, 0);
            if (!BCRYPT_SUCCESS(status)) {
                throw std::runtime_error("无法完成HMAC计算");
            }
            
            std::stringstream ss;
            for (DWORD i = 0; i < hashSize; i++) {
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hmacBuffer[i]);
            }
            result = ss.str();
            
        } catch (...) {
            if (hHash) BCryptDestroyHash(hHash);
            if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
            throw;
        }
        
        if (hHash) BCryptDestroyHash(hHash);
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
        
        return result;
    }
    
    bool verify_signature(const std::string& public_key, 
                         const std::string& data, 
                         const std::string& signature) {
        std::string computed_signature = sign_transaction(public_key, data);
        return computed_signature == signature;
    }
}