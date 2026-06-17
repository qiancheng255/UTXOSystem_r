/**
 * 未完成交易数据库实现文件
 * 记录每个用户的未完成交易，用于接收方验证和确认
 */

#include "pending_transaction.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

PendingTransactionDB::PendingTransactionDB() {}

void PendingTransactionDB::add_pending_transaction(const PendingTransactionRecord& record) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    // 添加到用户未完成交易列表
    user_pending_txs[record.receiver_username].push_back(record);
    
    // 添加到交易哈希映射
    tx_hash_to_record[record.tx_hash] = record;
    
    std::cout << "未完成交易已记录: " << record.tx_hash << std::endl;
    std::cout << "  发送方: " << record.sender_username << std::endl;
    std::cout << "  接收方: " << record.receiver_username << std::endl;
    std::cout << "  金额: " << record.amount << std::endl;
}

std::vector<PendingTransactionRecord> PendingTransactionDB::get_user_pending_transactions(const std::string& username) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = user_pending_txs.find(username);
    if (it != user_pending_txs.end()) {
        return it->second;
    }
    return {};
}

std::vector<PendingTransactionRecord> PendingTransactionDB::get_pending_received_transactions(const std::string& username) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<PendingTransactionRecord> result;
    auto it = user_pending_txs.find(username);
    if (it != user_pending_txs.end()) {
        for (const auto& record : it->second) {
            if (record.receiver_username == username && record.signature_status == "pending") {
                result.push_back(record);
            }
        }
    }
    return result;
}

std::vector<PendingTransactionRecord> PendingTransactionDB::get_pending_sent_transactions(const std::string& username) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<PendingTransactionRecord> result;
    for (const auto& pair : user_pending_txs) {
        for (const auto& record : pair.second) {
            if (record.sender_username == username) {
                result.push_back(record);
            }
        }
    }
    return result;
}

bool PendingTransactionDB::update_signature_status(const std::string& tx_hash, const std::string& status) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = tx_hash_to_record.find(tx_hash);
    if (it == tx_hash_to_record.end()) {
        std::cout << "交易不存在: " << tx_hash << std::endl;
        return false;
    }
    
    // 更新状态
    it->second.signature_status = status;
    
    // 同步更新用户列表中的记录
    std::string receiver = it->second.receiver_username;
    auto user_it = user_pending_txs.find(receiver);
    if (user_it != user_pending_txs.end()) {
        for (auto& record : user_it->second) {
            if (record.tx_hash == tx_hash) {
                record.signature_status = status;
                break;
            }
        }
    }
    
    std::cout << "交易 " << tx_hash << " 状态已更新为: " << status << std::endl;
    return true;
}

const PendingTransactionRecord* PendingTransactionDB::get_transaction_record(const std::string& tx_hash) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = tx_hash_to_record.find(tx_hash);
    if (it != tx_hash_to_record.end()) {
        return &(it->second);
    }
    return nullptr;
}

void PendingTransactionDB::remove_completed_transaction(const std::string& tx_hash) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = tx_hash_to_record.find(tx_hash);
    if (it != tx_hash_to_record.end()) {
        std::string receiver = it->second.receiver_username;
        
        // 从用户列表中移除
        auto user_it = user_pending_txs.find(receiver);
        if (user_it != user_pending_txs.end()) {
            auto& records = user_it->second;
            records.erase(
                std::remove_if(records.begin(), records.end(),
                    [&tx_hash](const PendingTransactionRecord& r) {
                        return r.tx_hash == tx_hash;
                    }),
                records.end()
            );
            
            // 如果用户没有未完成交易，移除用户条目
            if (records.empty()) {
                user_pending_txs.erase(user_it);
            }
        }
        
        // 从哈希映射中移除
        tx_hash_to_record.erase(it);
        
        std::cout << "交易 " << tx_hash << " 已从数据库中移除" << std::endl;
    }
}

int PendingTransactionDB::get_pending_count(const std::string& username) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    auto it = user_pending_txs.find(username);
    if (it != user_pending_txs.end()) {
        return static_cast<int>(it->second.size());
    }
    return 0;
}

std::vector<PendingTransactionRecord> PendingTransactionDB::get_all_pending_transactions() const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<PendingTransactionRecord> result;
    for (const auto& pair : tx_hash_to_record) {
        result.push_back(pair.second);
    }
    return result;
}

bool PendingTransactionDB::transaction_exists(const std::string& tx_hash) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    return tx_hash_to_record.find(tx_hash) != tx_hash_to_record.end();
}

std::string PendingTransactionDB::export_user_pending_report(const std::string& username) const {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::stringstream ss;
    ss << "\n==================================================" << std::endl;
    ss << "用户 " << username << " 的未完成交易报告" << std::endl;
    ss << "==================================================" << std::endl;
    
    // 待接收交易
    ss << "\n待接收交易:" << std::endl;
    ss << "--------------------------------------------------" << std::endl;
    
    int received_count = 0;
    auto user_it = user_pending_txs.find(username);
    if (user_it != user_pending_txs.end()) {
        for (const auto& record : user_it->second) {
            if (record.receiver_username == username && record.signature_status == "pending") {
                received_count++;
                ss << "\n交易哈希: " << record.tx_hash << std::endl;
                ss << "发送方: " << record.sender_username << " (" << record.sender_address << ")" << std::endl;
                ss << "金额: " << record.amount << " 单位" << std::endl;
                ss << "签名状态: " << record.signature_status << std::endl;
                
                time_t tx_time = static_cast<time_t>(record.timestamp);
                char time_str[26];
                ctime_s(time_str, sizeof(time_str), &tx_time);
                ss << "时间: " << time_str;
            }
        }
    }
    
    if (received_count == 0) {
        ss << "  无待接收交易" << std::endl;
    } else {
        ss << "\n共 " << received_count << " 笔待接收交易" << std::endl;
    }
    
    // 待发送交易（等待对方确认）
    ss << "\n待发送交易（等待对方确认）:" << std::endl;
    ss << "--------------------------------------------------" << std::endl;
    
    int sent_count = 0;
    for (const auto& pair : user_pending_txs) {
        for (const auto& record : pair.second) {
            if (record.sender_username == username) {
                sent_count++;
                ss << "\n交易哈希: " << record.tx_hash << std::endl;
                ss << "接收方: " << record.receiver_username << " (" << record.receiver_address << ")" << std::endl;
                ss << "金额: " << record.amount << " 单位" << std::endl;
                ss << "签名状态: " << record.signature_status << std::endl;
            }
        }
    }
    
    if (sent_count == 0) {
        ss << "  无待发送交易" << std::endl;
    } else {
        ss << "\n共 " << sent_count << " 笔待发送交易" << std::endl;
    }
    
    ss << "==================================================" << std::endl;
    
    return ss.str();
}