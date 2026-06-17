/**
 * 未完成交易数据库头文件
 * 记录每个用户的未完成交易，用于接收方验证和确认
 */

#ifndef PENDING_TRANSACTION_H
#define PENDING_TRANSACTION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "utxo.h"

// 未完成交易记录
struct PendingTransactionRecord {
    std::string tx_hash;           // 交易哈希
    std::string sender_address;    // 发送方地址
    std::string sender_username;   // 发送方用户名
    std::string receiver_address;  // 接收方地址
    std::string receiver_username; // 接收方用户名
    double amount;                 // 交易金额
    double timestamp;              // 交易时间戳
    std::string encrypted_signature; // 加密的数字签名
    std::string signature_status;  // 签名状态: "pending", "verified", "rejected"
    std::string verification_key;  // 验证密钥（用于接收方解密）
};

// 未完成交易数据库
class PendingTransactionDB {
private:
    // 用户未完成交易映射: username -> 未完成交易列表
    std::unordered_map<std::string, std::vector<PendingTransactionRecord>> user_pending_txs;
    
    // 交易哈希到记录的映射
    std::unordered_map<std::string, PendingTransactionRecord> tx_hash_to_record;
    
    mutable std::mutex db_mutex;
    
public:
    PendingTransactionDB();
    
    /**
     * 添加未完成交易
     * @param record 未完成交易记录
     */
    void add_pending_transaction(const PendingTransactionRecord& record);
    
    /**
     * 获取用户的未完成交易列表
     * @param username 用户名
     * @return 未完成交易列表
     */
    std::vector<PendingTransactionRecord> get_user_pending_transactions(const std::string& username) const;
    
    /**
     * 获取待接收的交易（用户作为接收方）
     * @param username 用户名
     * @return 待接收交易列表
     */
    std::vector<PendingTransactionRecord> get_pending_received_transactions(const std::string& username) const;
    
    /**
     * 获取待发送的交易（用户作为发送方）
     * @param username 用户名
     * @return 待发送交易列表
     */
    std::vector<PendingTransactionRecord> get_pending_sent_transactions(const std::string& username) const;
    
    /**
     * 更新交易签名状态
     * @param tx_hash 交易哈希
     * @param status 新状态
     * @return 是否更新成功
     */
    bool update_signature_status(const std::string& tx_hash, const std::string& status);
    
    /**
     * 获取交易记录
     * @param tx_hash 交易哈希
     * @return 交易记录指针
     */
    const PendingTransactionRecord* get_transaction_record(const std::string& tx_hash) const;
    
    /**
     * 移除已完成的交易
     * @param tx_hash 交易哈希
     */
    void remove_completed_transaction(const std::string& tx_hash);
    
    /**
     * 获取用户未完成交易数量
     * @param username 用户名
     * @return 未完成交易数量
     */
    int get_pending_count(const std::string& username) const;
    
    /**
     * 获取所有未完成交易
     * @return 所有未完成交易列表
     */
    std::vector<PendingTransactionRecord> get_all_pending_transactions() const;
    
    /**
     * 检查交易是否存在
     * @param tx_hash 交易哈希
     * @return 是否存在
     */
    bool transaction_exists(const std::string& tx_hash) const;
    
    /**
     * 导出用户未完成交易报告
     * @param username 用户名
     * @return 报告字符串
     */
    std::string export_user_pending_report(const std::string& username) const;
};

#endif // PENDING_TRANSACTION_H