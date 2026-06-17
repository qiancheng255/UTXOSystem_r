/**
 * 用户认证模块实现文件
 * 实现用户和管理员身份管理、登录验证、黑名单制度
 */

#include "user_auth.h"
#include "crypto.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

// UserAccount实现
UserAccount::UserAccount() : role(UserRole::USER), status(UserStatus::ACTIVE),
                             registration_time(0.0), last_login_time(0.0), login_attempts(0) {}

UserAccount::UserAccount(const std::string& user, const std::string& pass_hash, 
                         const std::string& addr, UserRole r)
    : username(user), password_hash(pass_hash), address(addr), role(r),
      status(UserStatus::ACTIVE), registration_time(static_cast<double>(time(nullptr))),
      last_login_time(0.0), login_attempts(0) {}

// BlacklistRecord实现
BlacklistRecord::BlacklistRecord() : timestamp(0.0) {}

BlacklistRecord::BlacklistRecord(const std::string& user, const std::string& r, 
                                 const std::string& admin)
    : username(user), reason(r), added_by(admin), timestamp(static_cast<double>(time(nullptr))) {}

// AuthManager实现
AuthManager::AuthManager() {
    // 创建默认管理员账户
    register_admin("admin", "admin123");
}

std::string AuthManager::hash_password(const std::string& password) {
    return Crypto::sha256("salt_" + password + "_utxo");
}

bool AuthManager::verify_password(const std::string& password, const std::string& hash) {
    return hash_password(password) == hash;
}

bool AuthManager::register_user(const std::string& username, const std::string& password, 
                                const std::string& address) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (accounts.find(username) != accounts.end()) {
        return false;
    }
    
    std::string pass_hash = hash_password(password);
    UserAccount new_account(username, pass_hash, address, UserRole::USER);
    accounts[username] = new_account;
    
    return true;
}

bool AuthManager::register_admin(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (accounts.find(username) != accounts.end()) {
        return false;
    }
    
    std::string pass_hash = hash_password(password);
    UserAccount new_account(username, pass_hash, "ADMIN_ADDRESS", UserRole::ADMIN);
    accounts[username] = new_account;
    admin_accounts.insert(username);
    
    return true;
}

bool AuthManager::login(const std::string& username, const std::string& password) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = accounts.find(username);
    if (it == accounts.end()) {
        return false;
    }
    
    UserAccount& account = it->second;
    
    if (account.status == UserStatus::BLACKLISTED) {
        return false;
    }
    
    if (account.status == UserStatus::SUSPENDED) {
        return false;
    }
    
    if (!verify_password(password, account.password_hash)) {
        account.login_attempts++;
        return false;
    }
    
    account.last_login_time = static_cast<double>(time(nullptr));
    account.login_attempts = 0;
    
    return true;
}

bool AuthManager::is_admin(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mtx);
    return admin_accounts.find(username) != admin_accounts.end();
}

bool AuthManager::is_blacklisted(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = accounts.find(username);
    if (it != accounts.end()) {
        return it->second.status == UserStatus::BLACKLISTED;
    }
    return false;
}

bool AuthManager::transfer(const std::string& from_user, const std::string& to_user, double amount) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto from_it = accounts.find(from_user);
    auto to_it = accounts.find(to_user);
    
    if (from_it == accounts.end() || to_it == accounts.end()) {
        return false;
    }
    
    if (from_it->second.status != UserStatus::ACTIVE || 
        to_it->second.status != UserStatus::ACTIVE) {
        return false;
    }
    
    if (amount <= 0) {
        return false;
    }
    
    return true;
}

std::string AuthManager::get_user_info(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = accounts.find(username);
    if (it == accounts.end()) {
        return "用户不存在";
    }
    
    const UserAccount& account = it->second;
    std::stringstream ss;
    ss << "=== 用户信息 ===" << std::endl;
    ss << "用户名: " << account.username << std::endl;
    ss << "地址: " << account.address << std::endl;
    ss << "角色: " << (account.role == UserRole::ADMIN ? "管理员" : "普通用户") << std::endl;
    ss << "状态: ";
    switch (account.status) {
        case UserStatus::ACTIVE: ss << "活跃"; break;
        case UserStatus::SUSPENDED: ss << "暂停"; break;
        case UserStatus::BLACKLISTED: ss << "黑名单"; break;
    }
    ss << std::endl;
    
    time_t reg_time = static_cast<time_t>(account.registration_time);
    char time_str[26];
    ctime_s(time_str, sizeof(time_str), &reg_time);
    ss << "注册时间: " << time_str;
    
    if (account.last_login_time > 0) {
        time_t login_time = static_cast<time_t>(account.last_login_time);
        ctime_s(time_str, sizeof(time_str), &login_time);
        ss << "最后登录: " << time_str;
    } else {
        ss << "最后登录: 从未登录" << std::endl;
    }
    
    return ss.str();
}

std::string AuthManager::get_transaction_history(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = accounts.find(username);
    if (it == accounts.end()) {
        return "用户不存在";
    }
    
    std::stringstream ss;
    ss << "=== 交易历史 ===" << std::endl;
    ss << "用户: " << username << std::endl;
    ss << "(交易记录由区块链系统管理)" << std::endl;
    
    return ss.str();
}

bool AuthManager::remove_user(const std::string& admin, const std::string& target_user) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (admin_accounts.find(admin) == admin_accounts.end()) {
        return false;
    }
    
    if (admin_accounts.find(target_user) != admin_accounts.end()) {
        return false;
    }
    
    auto it = accounts.find(target_user);
    if (it == accounts.end()) {
        return false;
    }
    
    accounts.erase(it);
    return true;
}

bool AuthManager::blacklist_user(const std::string& admin, const std::string& target_user, 
                                 const std::string& reason) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (admin_accounts.find(admin) == admin_accounts.end()) {
        return false;
    }
    
    if (admin_accounts.find(target_user) != admin_accounts.end()) {
        return false;
    }
    
    auto it = accounts.find(target_user);
    if (it == accounts.end()) {
        return false;
    }
    
    it->second.status = UserStatus::BLACKLISTED;
    BlacklistRecord record(target_user, reason, admin);
    blacklist[target_user] = record;
    
    return true;
}

bool AuthManager::unblacklist_user(const std::string& admin, const std::string& target_user) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (admin_accounts.find(admin) == admin_accounts.end()) {
        return false;
    }
    
    auto it = accounts.find(target_user);
    if (it == accounts.end()) {
        return false;
    }
    
    it->second.status = UserStatus::ACTIVE;
    blacklist.erase(target_user);
    
    return true;
}

std::string AuthManager::get_all_users_info() const {
    std::lock_guard<std::mutex> lock(mtx);
    
    std::stringstream ss;
    ss << "=== 所有用户信息 ===" << std::endl;
    ss << "总用户数: " << accounts.size() << std::endl;
    ss << "管理员数: " << admin_accounts.size() << std::endl;
    ss << "普通用户数: " << (accounts.size() - admin_accounts.size()) << std::endl;
    ss << "黑名单用户数: " << blacklist.size() << std::endl;
    ss << std::endl;
    
    for (const auto& pair : accounts) {
        const UserAccount& account = pair.second;
        ss << "用户名: " << account.username << std::endl;
        ss << "  角色: " << (account.role == UserRole::ADMIN ? "管理员" : "普通用户") << std::endl;
        ss << "  状态: ";
        switch (account.status) {
            case UserStatus::ACTIVE: ss << "活跃"; break;
            case UserStatus::SUSPENDED: ss << "暂停"; break;
            case UserStatus::BLACKLISTED: ss << "黑名单"; break;
        }
        ss << std::endl;
        ss << "  地址: " << account.address << std::endl;
        ss << std::endl;
    }
    
    return ss.str();
}

std::string AuthManager::get_all_transactions() const {
    std::lock_guard<std::mutex> lock(mtx);
    
    std::stringstream ss;
    ss << "=== 所有交易信息 ===" << std::endl;
    ss << "(交易记录由区块链系统管理)" << std::endl;
    ss << "用户总数: " << accounts.size() << std::endl;
    
    return ss.str();
}

std::string AuthManager::get_blacklist() const {
    std::lock_guard<std::mutex> lock(mtx);
    
    std::stringstream ss;
    ss << "=== 黑名单 ===" << std::endl;
    ss << "黑名单用户数: " << blacklist.size() << std::endl;
    ss << std::endl;
    
    for (const auto& pair : blacklist) {
        const BlacklistRecord& record = pair.second;
        ss << "用户名: " << record.username << std::endl;
        ss << "  原因: " << record.reason << std::endl;
        ss << "  添加者: " << record.added_by << std::endl;
        
        time_t time = static_cast<time_t>(record.timestamp);
        char time_str[26];
        ctime_s(time_str, sizeof(time_str), &time);
        ss << "  时间: " << time_str;
        ss << std::endl;
    }
    
    return ss.str();
}

size_t AuthManager::get_user_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return accounts.size();
}

size_t AuthManager::get_admin_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return admin_accounts.size();
}

const UserAccount* AuthManager::get_account(const std::string& username) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = accounts.find(username);
    if (it != accounts.end()) return &(it->second);
    return nullptr;
}