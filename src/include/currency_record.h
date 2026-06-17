/**
 * 货币记录模块头文件
 * 实现货币交易的RSA加密存储
 * 每笔交易的哈希值与时间戳进行RSA加密后存储
 */

#ifndef CURRENCY_RECORD_H
#define CURRENCY_RECORD_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <ctime>

/**
 * 加密货币记录结构
 * 存储RSA加密后的交易哈希和时间戳
 */
struct EncryptedCurrencyRecord {
    std::string record_id;              // 记录唯一标识
    std::string tx_hash;                // 原始交易哈希
    double timestamp;                   // 原始时间戳
    std::string encrypted_tx_hash;      // RSA加密后的交易哈希
    std::string encrypted_timestamp;    // RSA加密后的时间戳
    std::string rsa_public_key;         // 用于加密的公钥
    std::string rsa_key_id;             // RSA密钥标识
    double amount;                      // 交易金额
    std::string owner_address;          // 货币持有者地址
    std::string source_type;            // 来源类型: "mining", "transfer", "reward"
    
    EncryptedCurrencyRecord();
};

/**
 * 用户货币余额记录
 */
struct UserBalanceRecord {
    std::string user_address;           // 用户地址
    double total_balance;               // 总余额
    double frozen_balance;              // 冻结余额（待确认交易）
    int total_transactions;             // 总交易数
    double last_update_time;            // 最后更新时间
    std::vector<std::string> record_ids; // 关联的记录ID列表
    
    UserBalanceRecord();
    UserBalanceRecord(const std::string& addr);
};

/**
 * 货币记录数据库
 * 管理所有加密货币记录和用户余额
 */
class CurrencyRecordDB {
private:
    // 加密记录存储: record_id -> EncryptedCurrencyRecord
    std::unordered_map<std::string, EncryptedCurrencyRecord> encrypted_records;
    
    // 用户余额记录: address -> UserBalanceRecord
    std::unordered_map<std::string, UserBalanceRecord> user_balances;
    
    // 用户记录索引: address -> record_id列表
    std::unordered_map<std::string, std::vector<std::string>> user_records;
    
    // RSA密钥对缓存
    std::string system_public_key;
    std::string system_private_key;
    std::string system_key_id;
    
    mutable std::mutex db_mutex;
    int record_counter;
    
    std::string generate_record_id();
    void initialize_rsa_keys();
    
public:
    CurrencyRecordDB();
    
    /**
     * 记录货币获取（挖矿、奖励等）
     * @param owner_address 货币持有者地址
     * @param amount 金额
     * @param tx_hash 交易哈希
     * @param source_type 来源类型
     * @return 记录ID
     */
    std::string record_currency_acquisition(const std::string& owner_address,
                                           double amount,
                                           const std::string& tx_hash,
                                           const std::string& source_type);
    
    /**
     * 记录货币转移（转账）
     * @param sender_address 发送方地址
     * @param receiver_address 接收方地址
     * @param amount 金额
     * @param tx_hash 交易哈希
     * @return 记录ID
     */
    std::string record_currency_transfer(const std::string& sender_address,
                                        const std::string& receiver_address,
                                        double amount,
                                        const std::string& tx_hash);
    
    /**
     * 获取用户的加密货币记录列表
     * @param address 用户地址
     * @return 记录列表
     */
    std::vector<EncryptedCurrencyRecord> get_user_records(const std::string& address) const;
    
    /**
     * 获取用户余额
     * @param address 用户地址
     * @return 余额信息
     */
    UserBalanceRecord get_user_balance(const std::string& address) const;
    
    /**
     * 更新用户余额
     * @param address 用户地址
     * @param amount 变动金额（正数增加，负数减少）
     * @return 是否更新成功
     */
    bool update_user_balance(const std::string& address, double amount);
    
    /**
     * 验证加密记录的完整性
     * @param record_id 记录ID
     * @return 记录是否有效
     */
    bool verify_record_integrity(const std::string& record_id) const;
    
    /**
     * 解密并验证交易记录
     * @param record_id 记录ID
     * @param private_key 私钥
     * @return 解密后的数据（哈希|时间戳）
     */
    std::string decrypt_record(const std::string& record_id,
                              const std::string& private_key) const;
    
    /**
     * 获取系统公钥（用于外部验证）
     * @return 系统公钥
     */
    std::string get_system_public_key() const;
    
    /**
     * 获取系统密钥ID
     * @return 密钥ID
     */
    std::string get_system_key_id() const;
    
    /**
     * 获取总记录数
     * @return 记录数
     */
    size_t get_total_record_count() const;
    
    /**
     * 获取总用户数
     * @return 用户数
     */
    size_t get_total_user_count() const;
    
    /**
     * 导出用户货币报告
     * @param address 用户地址
     * @return 报告字符串
     */
    std::string export_user_currency_report(const std::string& address) const;
    
    /**
     * 导出系统统计报告
     * @return 报告字符串
     */
    std::string export_system_statistics() const;
};

#endif // CURRENCY_RECORD_H
