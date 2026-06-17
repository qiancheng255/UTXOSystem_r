/**
 * 用户信用体系头文件
 * 实现基于见证机制的用户信用系统
 * 新用户需要见证所有现有用户交易后才能进行正常交易
 */

#ifndef CREDIT_SYSTEM_H
#define CREDIT_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <ctime>

/**
 * 交易广播信息
 * 记录系统中广播的所有交易
 */
struct BroadcastTransaction {
    std::string tx_hash;
    std::string sender_address;
    std::string receiver_address;
    double amount;
    double timestamp;
    std::string block_hash;
    bool is_confirmed;
    
    BroadcastTransaction();
    BroadcastTransaction(const std::string& hash, const std::string& sender, 
                        const std::string& receiver, double amt);
};

/**
 * 用户见证记录
 * 记录用户见证的交易信息
 */
struct WitnessRecord {
    std::string tx_hash;
    double witness_timestamp;
    
    WitnessRecord();
    WitnessRecord(const std::string& hash);
};

/**
 * 用户信用信息
 */
struct UserCredit {
    std::string address;
    double registration_timestamp;
    bool is_active;
    std::unordered_set<std::string> witnessed_transactions;
    std::vector<WitnessRecord> witness_history;
    int required_witness_count;
    double credit_score;
    
    UserCredit();
    UserCredit(const std::string& addr, int required_count);
};

/**
 * 信用管理器类
 * 管理所有用户的信用信息和交易广播
 */
class CreditManager {
private:
    std::unordered_map<std::string, UserCredit> user_credits;
    std::unordered_map<std::string, BroadcastTransaction> broadcast_transactions;
    std::unordered_set<std::string> active_users;
    std::queue<std::string> pending_witness_queue;
    mutable std::mutex mtx;
    int min_witness_required;
    
    void update_user_credit_score(const std::string& address);
    bool check_witness_completion(const std::string& address);
    
public:
    CreditManager(int min_witness = 0);
    
    bool register_user(const std::string& address);
    bool broadcast_transaction(const std::string& tx_hash, 
                              const std::string& sender,
                              const std::string& receiver, 
                              double amount);
    bool witness_transaction(const std::string& address, const std::string& tx_hash);
    int batch_witness_transactions(const std::string& address, 
                                   const std::vector<std::string>& tx_hashes);
    bool can_user_trade(const std::string& address) const;
    const UserCredit* get_user_credit(const std::string& address) const;
    std::pair<int, int> get_witness_progress(const std::string& address) const;
    std::vector<std::string> get_pending_witness_transactions(const std::string& address = "") const;
    bool confirm_transaction(const std::string& tx_hash, const std::string& block_hash);
    std::unordered_set<std::string> get_active_users() const;
    size_t get_total_user_count() const;
    size_t get_active_user_count() const;
    size_t get_broadcast_transaction_count() const;
};

#endif // CREDIT_SYSTEM_H