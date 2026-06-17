/**
 * 用户认证模块头文件
 * 实现用户和管理员身份管理、登录验证、黑名单制度
 */

#ifndef USER_AUTH_H
#define USER_AUTH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <ctime>

// 用户角色枚举
enum class UserRole {
    USER,       // 普通用户
    ADMIN       // 管理员
};

// 用户状态枚举
enum class UserStatus {
    ACTIVE,     // 活跃
    SUSPENDED,  // 暂停
    BLACKLISTED // 黑名单
};

// 用户账户信息
struct UserAccount {
    std::string username;
    std::string password_hash;
    std::string address;
    UserRole role;
    UserStatus status;
    double registration_time;
    double last_login_time;
    int login_attempts;
    
    UserAccount();
    UserAccount(const std::string& user, const std::string& pass_hash, 
                const std::string& addr, UserRole r);
};

// 黑名单记录
struct BlacklistRecord {
    std::string username;
    std::string reason;
    std::string added_by;
    double timestamp;
    
    BlacklistRecord();
    BlacklistRecord(const std::string& user, const std::string& r, 
                    const std::string& admin);
};

// 认证管理器类
class AuthManager {
private:
    std::unordered_map<std::string, UserAccount> accounts;
    std::unordered_map<std::string, BlacklistRecord> blacklist;
    std::unordered_set<std::string> admin_accounts;
    mutable std::mutex mtx;
    
    std::string hash_password(const std::string& password);
    bool verify_password(const std::string& password, const std::string& hash);
    
public:
    AuthManager();
    
    // 注册功能
    bool register_user(const std::string& username, const std::string& password, 
                       const std::string& address);
    bool register_admin(const std::string& username, const std::string& password);
    
    // 登录功能
    bool login(const std::string& username, const std::string& password);
    bool is_admin(const std::string& username) const;
    bool is_blacklisted(const std::string& username) const;
    
    // 用户功能
    bool transfer(const std::string& from_user, const std::string& to_user, double amount);
    std::string get_user_info(const std::string& username) const;
    std::string get_transaction_history(const std::string& username) const;
    
    // 管理员功能
    bool remove_user(const std::string& admin, const std::string& target_user);
    bool blacklist_user(const std::string& admin, const std::string& target_user, 
                        const std::string& reason);
    bool unblacklist_user(const std::string& admin, const std::string& target_user);
    std::string get_all_users_info() const;
    std::string get_all_transactions() const;
    std::string get_blacklist() const;
    
    // 辅助功能
    size_t get_user_count() const;
    size_t get_admin_count() const;
    const UserAccount* get_account(const std::string& username) const;
};

#endif // USER_AUTH_H