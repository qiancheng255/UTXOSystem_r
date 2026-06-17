#pragma once
/**
 * @file privacy_utils.h
 * @brief 隐私保护工具模块 - 匿名地址生成与映射
 * 
 * 功能:
 * 1. 生成随机匿名地址（时间戳 + 哈希随机拼合）
 * 2. 维护真实地址与匿名地址的映射表
 * 3. 防止通过多次交易定位交易发起方
 */

#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <chrono>
#include <mutex>

/**
 * @class PrivacyUtils
 * @brief 隐私保护工具类
 */
class PrivacyUtils {
public:
    /**
     * @brief 生成随机匿名地址
     * @return 匿名地址字符串
     */
    static std::string generate_anonymous_address();

    /**
     * @brief 注册真实地址与匿名地址的映射
     * @param real_address 真实地址
     * @return 匿名地址
     */
    static std::string register_mapping(const std::string& real_address);

    /**
     * @brief 根据真实地址获取匿名地址
     * @param real_address 真实地址
     * @return 匿名地址，如果不存在则返回空字符串
     */
    static std::string get_anonymous_address(const std::string& real_address);

    /**
     * @brief 根据匿名地址获取真实地址
     * @param anon_address 匿名地址
     * @return 真实地址，如果不存在则返回空字符串
     */
    static std::string get_real_address(const std::string& anon_address);

    /**
     * @brief 检查是否是匿名地址
     * @param address 地址
     * @return 是否是匿名地址
     */
    static bool is_anonymous_address(const std::string& address);

    /**
     * @brief 解析地址（如果是匿名地址则返回真实地址）
     * @param address 地址（可能是匿名的）
     * @return 真实地址
     */
    static std::string resolve_address(const std::string& address);

    /**
     * @brief 获取所有映射关系
     * @return 映射表
     */
    static std::unordered_map<std::string, std::string> get_all_mappings();

    /**
     * @brief 为指定用户生成新的匿名地址（用于不同交易）
     * @param real_address 真实地址
     * @param transaction_hash 交易哈希（用于生成唯一匿名地址）
     * @return 新的匿名地址
     */
    static std::string generate_transaction_anonymous_address(
        const std::string& real_address,
        const std::string& transaction_hash);

private:
    // 内部生成匿名地址（调用者需已持有锁）
    static std::string generate_anonymous_address_internal();
    
    // 真实地址 -> 匿名地址 映射
    static std::unordered_map<std::string, std::string> real_to_anon_;
    
    // 匿名地址 -> 真实地址 映射
    static std::unordered_map<std::string, std::string> anon_to_real_;
    
    // 每个真实地址的所有匿名地址（用于同一用户的多次交易）
    static std::unordered_map<std::string, std::vector<std::string>> user_anon_addresses_;
    
    static std::mutex mutex_;
};
