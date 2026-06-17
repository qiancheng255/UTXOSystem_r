/**
 * 用户数据库实现文件
 * 实现60人分组管理的用户数据库系统
 */

#include "user_database.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>

// UserInfo实现
UserInfo::UserInfo() : group_id(-1), registration_time(0.0), credit_score(0.0),
                       is_active(false), witnessed_count(0), required_witness_count(0),
                       status("pending") {}

UserInfo::UserInfo(const std::string& id, const std::string& addr, int gid)
    : user_id(id), address(addr), group_id(gid),
      registration_time(static_cast<double>(time(nullptr))),
      credit_score(0.0), is_active(false), witnessed_count(0),
      required_witness_count(0), status("pending") {}

// TransactionRecord实现
TransactionRecord::TransactionRecord() : amount(0.0), timestamp(0.0), sender_group(-1), 
                                         receiver_group(-1), status("pending") {}

// UserGroup实现
UserGroup::UserGroup() : group_id(-1), max_capacity(GROUP_SIZE), 
                         creation_time(0.0), group_credit_score(0.0) {}

UserGroup::UserGroup(int id) : group_id(id), max_capacity(GROUP_SIZE),
                               creation_time(static_cast<double>(time(nullptr))),
                               group_credit_score(0.0) {}

bool UserGroup::is_full() const { return user_ids.size() >= max_capacity; }
int UserGroup::current_size() const { return static_cast<int>(user_ids.size()); }

// UserDatabase实现
UserDatabase::UserDatabase(const std::string& path) 
    : db_path(path), next_group_id(0), user_counter(0) {
    std::filesystem::create_directories(db_path);
}

UserDatabase::~UserDatabase() {}

std::string UserDatabase::generate_user_id() {
    user_counter++;
    std::stringstream ss;
    ss << "USER_" << std::setw(6) << std::setfill('0') << user_counter;
    return ss.str();
}

int UserDatabase::find_available_group() {
    for (auto& pair : groups) {
        if (!pair.second.is_full()) return pair.first;
    }
    if (groups.size() < MAX_GROUPS) {
        int new_group_id = next_group_id++;
        groups[new_group_id] = UserGroup(new_group_id);
        return new_group_id;
    }
    return -1;
}

void UserDatabase::update_group_credit_score(int group_id) {
    auto it = groups.find(group_id);
    if (it == groups.end()) return;
    
    UserGroup& group = it->second;
    double total_credit = 0.0;
    int user_count = 0;
    
    for (const auto& user_id : group.user_ids) {
        auto user_it = users.find(user_id);
        if (user_it != users.end()) {
            total_credit += user_it->second.credit_score;
            user_count++;
        }
    }
    
    group.group_credit_score = (user_count > 0) ? (total_credit / user_count) : 0.0;
}

size_t UserDatabase::get_active_user_count_internal() const {
    size_t count = 0;
    for (const auto& pair : users) {
        if (pair.second.is_active) count++;
    }
    return count;
}

std::string UserDatabase::register_user(const std::string& address) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (address_to_id.find(address) != address_to_id.end()) return "";
    if (users.size() >= MAX_USERS) return "";
    
    int group_id = find_available_group();
    if (group_id == -1) return "";
    
    std::string user_id = generate_user_id();
    UserInfo new_user(user_id, address, group_id);
    new_user.required_witness_count = static_cast<int>(transactions.size());
    
    if (new_user.required_witness_count == 0) {
        new_user.is_active = true;
        new_user.status = "active";
        new_user.credit_score = 100.0;
    }
    
    users[user_id] = new_user;
    address_to_id[address] = user_id;
    groups[group_id].user_ids.push_back(user_id);
    update_group_credit_score(group_id);
    
    return user_id;
}

const UserInfo* UserDatabase::get_user(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = users.find(user_id);
    if (it != users.end()) return &(it->second);
    return nullptr;
}

const UserInfo* UserDatabase::get_user_by_address(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = address_to_id.find(address);
    if (it != address_to_id.end()) {
        auto user_it = users.find(it->second);
        if (user_it != users.end()) return &(user_it->second);
    }
    return nullptr;
}

bool UserDatabase::record_transaction(const std::string& tx_id,
                                      const std::string& sender_address,
                                      const std::string& receiver_address,
                                      double amount) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (transactions.find(tx_id) != transactions.end()) return false;
    
    auto sender_it = address_to_id.find(sender_address);
    auto receiver_it = address_to_id.find(receiver_address);
    
    std::string sender_id = (sender_it != address_to_id.end()) ? sender_it->second : "";
    std::string receiver_id = (receiver_it != address_to_id.end()) ? receiver_it->second : "";
    
    int sender_group = -1;
    int receiver_group = -1;
    
    if (!sender_id.empty()) sender_group = users[sender_id].group_id;
    if (!receiver_id.empty()) receiver_group = users[receiver_id].group_id;
    
    TransactionRecord record;
    record.tx_id = tx_id;
    record.sender_id = sender_id;
    record.receiver_id = receiver_id;
    record.sender_address = sender_address;
    record.receiver_address = receiver_address;
    record.amount = amount;
    record.timestamp = static_cast<double>(time(nullptr));
    record.sender_group = sender_group;
    record.receiver_group = receiver_group;
    
    transactions[tx_id] = record;
    
    if (!sender_id.empty()) user_transactions[sender_id].push_back(tx_id);
    if (!receiver_id.empty()) user_transactions[receiver_id].push_back(tx_id);
    
    return true;
}

bool UserDatabase::confirm_transaction(const std::string& tx_id, const std::string& block_hash) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = transactions.find(tx_id);
    if (it == transactions.end()) return false;
    it->second.block_hash = block_hash;
    it->second.status = "confirmed";
    return true;
}

bool UserDatabase::witness_transaction(const std::string& user_id, const std::string& tx_id) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto user_it = users.find(user_id);
    if (user_it == users.end()) return false;
    if (user_it->second.is_active) return true;
    
    if (transactions.find(tx_id) == transactions.end()) return false;
    if (user_witnessed[user_id].find(tx_id) != user_witnessed[user_id].end()) return false;
    
    user_witnessed[user_id].insert(tx_id);
    user_it->second.witnessed_count = static_cast<int>(user_witnessed[user_id].size());
    
    if (user_it->second.required_witness_count > 0) {
        double progress = static_cast<double>(user_it->second.witnessed_count) /
                         static_cast<double>(user_it->second.required_witness_count);
        user_it->second.credit_score = 50.0 + (progress * 50.0);
    }
    
    if (user_it->second.witnessed_count >= user_it->second.required_witness_count) {
        user_it->second.is_active = true;
        user_it->second.status = "active";
        user_it->second.credit_score = 100.0;
        update_group_credit_score(user_it->second.group_id);
    }
    
    return true;
}

bool UserDatabase::can_user_trade(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = users.find(user_id);
    if (it == users.end()) return false;
    return it->second.is_active;
}

std::pair<int, int> UserDatabase::get_witness_progress(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = users.find(user_id);
    if (it == users.end()) return {0, 0};
    return {it->second.witnessed_count, it->second.required_witness_count};
}

std::vector<std::string> UserDatabase::get_pending_witness_transactions(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> pending;
    
    auto user_it = users.find(user_id);
    if (user_it != users.end()) {
        for (const auto& pair : transactions) {
            if (user_witnessed.at(user_id).find(pair.first) == 
                user_witnessed.at(user_id).end()) {
                pending.push_back(pair.first);
            }
        }
    }
    
    return pending;
}

size_t UserDatabase::get_total_user_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return users.size();
}

size_t UserDatabase::get_active_user_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return get_active_user_count_internal();
}

size_t UserDatabase::get_total_transaction_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return transactions.size();
}

size_t UserDatabase::get_group_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return groups.size();
}

std::string UserDatabase::export_user_report(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = users.find(user_id);
    if (it == users.end()) return "用户不存在";
    
    const UserInfo& user = it->second;
    std::stringstream ss;
    ss << "=== 用户报告 ===" << std::endl;
    ss << "用户ID: " << user.user_id << std::endl;
    ss << "地址: " << user.address << std::endl;
    ss << "所属组: " << user.group_id << std::endl;
    ss << "状态: " << user.status << std::endl;
    ss << "信用分数: " << std::fixed << std::setprecision(2) << user.credit_score << std::endl;
    ss << "见证进度: " << user.witnessed_count << "/" << user.required_witness_count << std::endl;
    
    return ss.str();
}

std::string UserDatabase::export_group_report(int group_id) const {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = groups.find(group_id);
    if (it == groups.end()) return "组不存在";
    
    const UserGroup& group = it->second;
    std::stringstream ss;
    ss << "=== 组报告 ===" << std::endl;
    ss << "组ID: " << group.group_id << std::endl;
    ss << "用户数量: " << group.current_size() << "/" << group.max_capacity << std::endl;
    ss << "组信用分数: " << std::fixed << std::setprecision(2) << group.group_credit_score << std::endl;
    
    return ss.str();
}

std::string UserDatabase::export_statistics_report() const {
    std::lock_guard<std::mutex> lock(mtx);
    
    std::stringstream ss;
    ss << "=== 数据库统计报告 ===" << std::endl;
    ss << "总用户数: " << users.size() << "/" << MAX_USERS << std::endl;
    ss << "活跃用户数: " << get_active_user_count_internal() << std::endl;
    ss << "组数量: " << groups.size() << "/" << MAX_GROUPS << std::endl;
    ss << "每组容量: " << GROUP_SIZE << std::endl;
    ss << "总交易数: " << transactions.size() << std::endl;
    
    double total_credit = 0.0;
    for (const auto& pair : users) {
        total_credit += pair.second.credit_score;
    }
    double avg_credit = users.empty() ? 0.0 : total_credit / users.size();
    ss << "平均信用分数: " << std::fixed << std::setprecision(2) << avg_credit << std::endl;
    
    return ss.str();
}