/**
 * @file privacy_utils.cpp
 * @brief 隐私保护工具实现 - 匿名地址生成与映射
 */

#include "privacy_utils.h"
#include "crypto.h"
#include <sstream>
#include <iomanip>
#include <thread>

// 静态成员初始化
std::unordered_map<std::string, std::string> PrivacyUtils::real_to_anon_;
std::unordered_map<std::string, std::string> PrivacyUtils::anon_to_real_;
std::unordered_map<std::string, std::vector<std::string>> PrivacyUtils::user_anon_addresses_;
std::mutex PrivacyUtils::mutex_;

std::string PrivacyUtils::generate_anonymous_address() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 获取高精度时间戳（纳秒级）
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // 生成随机数
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
    uint64_t random_value = dis(gen);
    
    // 混合时间戳和随机数
    std::stringstream ss;
    ss << nanos << "_" << random_value << "_" << std::this_thread::get_id();
    std::string seed = ss.str();
    
    // 使用SHA256生成匿名地址
    std::string hash = Crypto::sha256(seed);
    
    // 添加前缀标识匿名地址
    return "anon_" + hash.substr(0, 40);
}

std::string PrivacyUtils::register_mapping(const std::string& real_address) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查是否已存在映射
    auto it = real_to_anon_.find(real_address);
    if (it != real_to_anon_.end()) {
        return it->second;
    }
    
    // 生成新的匿名地址
    std::string anon_address = generate_anonymous_address_internal();
    
    // 建立双向映射
    real_to_anon_[real_address] = anon_address;
    anon_to_real_[anon_address] = real_address;
    user_anon_addresses_[real_address].push_back(anon_address);
    
    return anon_address;
}

std::string PrivacyUtils::get_anonymous_address(const std::string& real_address) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = real_to_anon_.find(real_address);
    if (it != real_to_anon_.end()) {
        return it->second;
    }
    return "";
}

std::string PrivacyUtils::get_real_address(const std::string& anon_address) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = anon_to_real_.find(anon_address);
    if (it != anon_to_real_.end()) {
        return it->second;
    }
    return "";
}

bool PrivacyUtils::is_anonymous_address(const std::string& address) {
    return address.substr(0, 5) == "anon_";
}

std::string PrivacyUtils::resolve_address(const std::string& address) {
    if (is_anonymous_address(address)) {
        std::string real = get_real_address(address);
        return real.empty() ? address : real;
    }
    return address;
}

std::unordered_map<std::string, std::string> PrivacyUtils::get_all_mappings() {
    std::lock_guard<std::mutex> lock(mutex_);
    return real_to_anon_;
}

std::string PrivacyUtils::generate_transaction_anonymous_address(
    const std::string& real_address,
    const std::string& transaction_hash) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 获取高精度时间戳
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // 生成随机数
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
    uint64_t random_value = dis(gen);
    
    // 混合真实地址、交易哈希、时间戳和随机数
    std::stringstream ss;
    ss << real_address << "_" << transaction_hash << "_" << nanos << "_" << random_value;
    std::string seed = ss.str();
    
    // 使用SHA256生成匿名地址
    std::string hash = Crypto::sha256(seed);
    std::string anon_address = "anon_" + hash.substr(0, 40);
    
    // 记录映射关系
    anon_to_real_[anon_address] = real_address;
    user_anon_addresses_[real_address].push_back(anon_address);
    
    return anon_address;
}

// 内部函数（已锁定）
std::string PrivacyUtils::generate_anonymous_address_internal() {
    // 获取高精度时间戳（纳秒级）
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    // 生成随机数
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, UINT64_MAX);
    uint64_t random_value = dis(gen);
    
    // 混合时间戳和随机数
    std::stringstream ss;
    ss << nanos << "_" << random_value << "_" << std::this_thread::get_id();
    std::string seed = ss.str();
    
    // 使用SHA256生成匿名地址
    std::string hash = Crypto::sha256(seed);
    
    // 添加前缀标识匿名地址
    return "anon_" + hash.substr(0, 40);
}
