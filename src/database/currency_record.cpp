/**
 * 货币记录模块实现文件
 * 实现货币交易的RSA加密存储
 */

#include "currency_record.h"
#include "rsa_crypto.h"
#include "crypto.h"
#include <sstream>
#include <iomanip>
#include <iostream>

// EncryptedCurrencyRecord实现
EncryptedCurrencyRecord::EncryptedCurrencyRecord() 
    : timestamp(0.0), amount(0.0) {}

// UserBalanceRecord实现
UserBalanceRecord::UserBalanceRecord() 
    : total_balance(0.0), frozen_balance(0.0), 
      total_transactions(0), last_update_time(0.0) {}

UserBalanceRecord::UserBalanceRecord(const std::string& addr)
    : user_address(addr), total_balance(0.0), frozen_balance(0.0),
      total_transactions(0), last_update_time(static_cast<double>(time(nullptr))) {}

// CurrencyRecordDB实现
CurrencyRecordDB::CurrencyRecordDB() : record_counter(0) {
    initialize_rsa_keys();
}

std::string CurrencyRecordDB::generate_record_id() {
    record_counter++;
    std::stringstream ss;
    ss << "CR_" << std::setw(8) << std::setfill('0') << record_counter;
    return ss.str();
}

void CurrencyRecordDB::initialize_rsa_keys() {
    // 生成系统RSA密钥对
    RSACrypto::RSAKeyPair key_pair = RSACrypto::generate_key_pair(2048);
    system_public_key = key_pair.public_key;
    system_private_key = key_pair.private_key;
    system_key_id = key_pair.key_id;
    
    std::cout << "[货币记录] RSA密钥对已生成，密钥ID: " << system_key_id << std::endl;
}

std::string CurrencyRecordDB::record_currency_acquisition(
    const std::string& owner_address,
    double amount,
    const std::string& tx_hash,
    const std::string& source_type) {
    
    std::lock_guard<std::mutex> lock(db_mutex);
    
    // 生成记录ID
    std::string record_id = generate_record_id();
    
    // 获取当前时间戳
    double timestamp = static_cast<double>(time(nullptr));
    std::string timestamp_str = std::to_string(timestamp);
    
    // RSA加密交易哈希
    std::string encrypted_hash = RSACrypto::encrypt_with_public_key(
        system_public_key, tx_hash);
    
    // RSA加密时间戳
    std::string encrypted_ts = RSACrypto::encrypt_with_public_key(
        system_public_key, timestamp_str);
    
    // 创建加密记录
    EncryptedCurrencyRecord record;
    record.record_id = record_id;
    record.tx_hash = tx_hash;
    record.timestamp = timestamp;
    record.encrypted_tx_hash = encrypted_hash;
    record.encrypted_timestamp = encrypted_ts;
    record.rsa_public_key = system_public_key;
    record.rsa_key_id = system_key_id;
    record.amount = amount;
    record.owner_address = owner_address;
    record.source_type = source_type;
    
    // 存储记录
    encrypted_records[record_id] = record;
    user_records[owner_address].push_back(record_id);
    
    // 更新用户余额
    auto balance_it = user_balances.find(owner_address);
    if (balance_it == user_balances.end()) {
        UserBalanceRecord new_balance(owner_address);
        new_balance.total_balance = amount;
        new_balance.total_transactions = 1;
        new_balance.record_ids.push_back(record_id);
        user_balances[owner_address] = new_balance;
    } else {
        balance_it->second.total_balance += amount;
        balance_it->second.total_transactions++;
        balance_it->second.last_update_time = timestamp;
        balance_it->second.record_ids.push_back(record_id);
    }
    
    std::cout << "[货币记录] 已记录货币获取: " << record_id 
              << " | 金额: " << amount 
              << " | 持有者: " << owner_address << std::endl;
    
    return record_id;
}

std::string CurrencyRecordDB::record_currency_transfer(
    const std::string& sender_address,
    const std::string& receiver_address,
    double amount,
    const std::string& tx_hash) {
    
    std::lock_guard<std::mutex> lock(db_mutex);
    
    // 检查发送方余额
    auto sender_it = user_balances.find(sender_address);
    if (sender_it == user_balances.end() || sender_it->second.total_balance < amount) {
        std::cout << "[货币记录] 转账失败: 发送方余额不足" << std::endl;
        return "";
    }
    
    // 生成记录ID
    std::string record_id = generate_record_id();
    
    // 获取当前时间戳
    double timestamp = static_cast<double>(time(nullptr));
    std::string timestamp_str = std::to_string(timestamp);
    
    // RSA加密交易哈希
    std::string encrypted_hash = RSACrypto::encrypt_with_public_key(
        system_public_key, tx_hash);
    
    // RSA加密时间戳
    std::string encrypted_ts = RSACrypto::encrypt_with_public_key(
        system_public_key, timestamp_str);
    
    // 创建加密记录（发送方）
    EncryptedCurrencyRecord sender_record;
    sender_record.record_id = record_id + "_SEND";
    sender_record.tx_hash = tx_hash;
    sender_record.timestamp = timestamp;
    sender_record.encrypted_tx_hash = encrypted_hash;
    sender_record.encrypted_timestamp = encrypted_ts;
    sender_record.rsa_public_key = system_public_key;
    sender_record.rsa_key_id = system_key_id;
    sender_record.amount = -amount; // 负数表示转出
    sender_record.owner_address = sender_address;
    sender_record.source_type = "transfer_out";
    
    // 创建加密记录（接收方）
    EncryptedCurrencyRecord receiver_record;
    receiver_record.record_id = record_id + "_RECV";
    receiver_record.tx_hash = tx_hash;
    receiver_record.timestamp = timestamp;
    receiver_record.encrypted_tx_hash = encrypted_hash;
    receiver_record.encrypted_timestamp = encrypted_ts;
    receiver_record.rsa_public_key = system_public_key;
    receiver_record.rsa_key_id = system_key_id;
    receiver_record.amount = amount; // 正数表示转入
    receiver_record.owner_address = receiver_address;
    receiver_record.source_type = "transfer_in";
    
    // 存储记录
    encrypted_records[sender_record.record_id] = sender_record;
    encrypted_records[receiver_record.record_id] = receiver_record;
    user_records[sender_address].push_back(sender_record.record_id);
    user_records[receiver_address].push_back(receiver_record.record_id);
    
    // 更新发送方余额
    sender_it->second.total_balance -= amount;
    sender_it->second.total_transactions++;
    sender_it->second.last_update_time = timestamp;
    sender_it->second.record_ids.push_back(sender_record.record_id);
    
    // 更新接收方余额
    auto receiver_it = user_balances.find(receiver_address);
    if (receiver_it == user_balances.end()) {
        UserBalanceRecord new_balance(receiver_address);
        new_balance.total_balance = amount;
        new_balance.total_transactions = 1;
        new_balance.last_update_time = timestamp;
        new_balance.record_ids.push_back(receiver_record.record_id);
        user_balances[receiver_address] = new_balance;
    } else {
        receiver_it->second.total_balance += amount;
        receiver_it->second.total_transactions++;
        receiver_it->second.last_update_time = timestamp;
        receiver_it->second.record_ids.push_back(receiver_record.record_id);
    }
    
    std::cout << "[货币记录] 已记录货币转移: " << record_id 
              << " | 金额: " << amount 
              << " | 从 " << sender_address 
              << " 到 " << receiver_address << std::endl;
    
    return record_id;
}

std::vector<EncryptedCurrencyRecord> CurrencyRecordDB::get_user_records(
    const std::string& address) const {
    
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<EncryptedCurrencyRecord> records;
    
    auto it = user_records.find(address);
    if (it != user_records.end()) {
        for (const auto& record_id : it->second) {
            auto record_it = encrypted_records.find(record_id);
            if (record_it != encrypted_records.end()) {
                records.push_back(record_it->second);
            }
        }
    }
    
    return records;
}

UserBalanceRecord CurrencyRecordDB::get_user_balance(const std::string& address) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = user_balances.find(address);
    if (it != user_balances.end()) {
        return it->second;
    }
    
    return UserBalanceRecord(address);
}

bool CurrencyRecordDB::update_user_balance(const std::string& address, double amount) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = user_balances.find(address);
    if (it == user_balances.end()) {
        if (amount < 0) return false; // 不能给不存在的用户减少余额
        UserBalanceRecord new_balance(address);
        new_balance.total_balance = amount;
        user_balances[address] = new_balance;
        return true;
    }
    
    if (amount < 0 && it->second.total_balance + amount < 0) {
        return false; // 余额不足
    }
    
    it->second.total_balance += amount;
    it->second.last_update_time = static_cast<double>(time(nullptr));
    return true;
}

bool CurrencyRecordDB::verify_record_integrity(const std::string& record_id) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = encrypted_records.find(record_id);
    if (it == encrypted_records.end()) return false;
    
    const auto& record = it->second;
    
    // 验证加密数据是否完整
    if (record.encrypted_tx_hash.empty() || record.encrypted_timestamp.empty()) {
        return false;
    }
    
    // 验证RSA密钥ID是否匹配
    if (record.rsa_key_id != system_key_id) {
        return false;
    }
    
    return true;
}

std::string CurrencyRecordDB::decrypt_record(const std::string& record_id,
                                            const std::string& private_key) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = encrypted_records.find(record_id);
    if (it == encrypted_records.end()) return "";
    
    const auto& record = it->second;
    
    // 使用私钥解密交易哈希
    std::string decrypted_hash = RSACrypto::decrypt_with_private_key(
        private_key, record.encrypted_tx_hash);
    
    // 使用私钥解密时间戳
    std::string decrypted_ts = RSACrypto::decrypt_with_private_key(
        private_key, record.encrypted_timestamp);
    
    // 返回解密后的数据
    return decrypted_hash + "|" + decrypted_ts;
}

std::string CurrencyRecordDB::get_system_public_key() const {
    return system_public_key;
}

std::string CurrencyRecordDB::get_system_key_id() const {
    return system_key_id;
}

size_t CurrencyRecordDB::get_total_record_count() const {
    std::lock_guard<std::mutex> lock(db_mutex);
    return encrypted_records.size();
}

size_t CurrencyRecordDB::get_total_user_count() const {
    std::lock_guard<std::mutex> lock(db_mutex);
    return user_balances.size();
}

std::string CurrencyRecordDB::export_user_currency_report(const std::string& address) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::stringstream ss;
    ss << "=== 用户货币报告 ===" << std::endl;
    ss << "用户地址: " << address << std::endl;
    
    auto balance_it = user_balances.find(address);
    if (balance_it != user_balances.end()) {
        const auto& balance = balance_it->second;
        ss << "总余额: " << std::fixed << std::setprecision(2) << balance.total_balance << " 单位" << std::endl;
        ss << "冻结余额: " << balance.frozen_balance << " 单位" << std::endl;
        ss << "可用余额: " << (balance.total_balance - balance.frozen_balance) << " 单位" << std::endl;
        ss << "总交易数: " << balance.total_transactions << std::endl;
        
        time_t last_update = static_cast<time_t>(balance.last_update_time);
        char time_str[26];
        ctime_s(time_str, sizeof(time_str), &last_update);
        ss << "最后更新: " << time_str;
    } else {
        ss << "未找到用户余额记录" << std::endl;
    }
    
    auto records_it = user_records.find(address);
    if (records_it != user_records.end()) {
        ss << "\n--- 加密记录列表 ---" << std::endl;
        for (const auto& record_id : records_it->second) {
            auto record_it = encrypted_records.find(record_id);
            if (record_it != encrypted_records.end()) {
                const auto& record = record_it->second;
                ss << "记录ID: " << record.record_id << std::endl;
                ss << "  交易哈希: " << record.tx_hash.substr(0, 16) << "..." << std::endl;
                ss << "  金额: " << record.amount << " 单位" << std::endl;
                ss << "  来源类型: " << record.source_type << std::endl;
                ss << "  RSA密钥ID: " << record.rsa_key_id << std::endl;
                ss << "  加密哈希: " << record.encrypted_tx_hash.substr(0, 32) << "..." << std::endl;
                ss << "  加密时间戳: " << record.encrypted_timestamp.substr(0, 32) << "..." << std::endl;
                ss << std::endl;
            }
        }
    }
    
    return ss.str();
}

std::string CurrencyRecordDB::export_system_statistics() const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::stringstream ss;
    ss << "=== 货币系统统计 ===" << std::endl;
    ss << "总记录数: " << encrypted_records.size() << std::endl;
    ss << "总用户数: " << user_balances.size() << std::endl;
    ss << "系统密钥ID: " << system_key_id << std::endl;
    
    double total_supply = 0.0;
    for (const auto& pair : user_balances) {
        total_supply += pair.second.total_balance;
    }
    ss << "货币总供应量: " << std::fixed << std::setprecision(2) << total_supply << " 单位" << std::endl;
    
    return ss.str();
}
