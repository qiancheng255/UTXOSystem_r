/**
 * 用户数据库管理头文件
 * 实现60人分组管理的用户数据库系统
 * 记录用户信息和交易信息
 */

#ifndef USER_DATABASE_H
#define USER_DATABASE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <ctime>

// 常量定义
constexpr int GROUP_SIZE = 60;
constexpr int MAX_GROUPS = 100;
constexpr int MAX_USERS = GROUP_SIZE * MAX_GROUPS;

/**
 * 用户信息结构
 */
struct UserInfo {
    std::string user_id;
    std::string address;
    int group_id;
    double registration_time;
    double credit_score;
    bool is_active;
    int witnessed_count;
    int required_witness_count;
    std::string status;
    
    UserInfo();
    UserInfo(const std::string& id, const std::string& addr, int gid);
};

/**
 * 交易记录结构
 */
struct TransactionRecord {
    std::string tx_id;
    std::string sender_id;
    std::string receiver_id;
    std::string sender_address;
    std::string receiver_address;
    double amount;
    double timestamp;
    std::string block_hash;
    std::string status;
    int sender_group;
    int receiver_group;
    
    TransactionRecord();
};

/**
 * 用户组信息
 */
struct UserGroup {
    int group_id;
    std::vector<std::string> user_ids;
    int max_capacity;
    double creation_time;
    double group_credit_score;
    
    UserGroup();
    UserGroup(int id);
    bool is_full() const;
    int current_size() const;
};

/**
 * 数据库管理器类
 * 管理用户数据库和交易记录
 */
class UserDatabase {
private:
    std::unordered_map<std::string, UserInfo> users;
    std::unordered_map<std::string, std::string> address_to_id;
    std::unordered_map<int, UserGroup> groups;
    std::unordered_map<std::string, TransactionRecord> transactions;
    std::unordered_map<std::string, std::vector<std::string>> user_transactions;
    std::unordered_map<std::string, std::unordered_set<std::string>> user_witnessed;
    int next_group_id;
    int user_counter;
    mutable std::mutex mtx;
    std::string db_path;
    
    std::string generate_user_id();
    int find_available_group();
    void update_group_credit_score(int group_id);
    size_t get_active_user_count_internal() const;
    
public:
    UserDatabase(const std::string& path = "user_db");
    ~UserDatabase();
    
    std::string register_user(const std::string& address);
    const UserInfo* get_user(const std::string& user_id) const;
    const UserInfo* get_user_by_address(const std::string& address) const;
    bool record_transaction(const std::string& tx_id,
                           const std::string& sender_address,
                           const std::string& receiver_address,
                           double amount);
    bool confirm_transaction(const std::string& tx_id, const std::string& block_hash);
    bool witness_transaction(const std::string& user_id, const std::string& tx_id);
    bool can_user_trade(const std::string& user_id) const;
    std::pair<int, int> get_witness_progress(const std::string& user_id) const;
    std::vector<std::string> get_pending_witness_transactions(const std::string& user_id) const;
    size_t get_total_user_count() const;
    size_t get_active_user_count() const;
    size_t get_total_transaction_count() const;
    size_t get_group_count() const;
    std::string export_user_report(const std::string& user_id) const;
    std::string export_group_report(int group_id) const;
    std::string export_statistics_report() const;
};

#endif // USER_DATABASE_H