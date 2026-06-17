/**
 * 用户信用体系实现文件
 * 实现基于见证机制的用户信用系统
 */

#include "credit_system.h"

// BroadcastTransaction实现
BroadcastTransaction::BroadcastTransaction() : amount(0.0), timestamp(0.0), is_confirmed(false) {}

BroadcastTransaction::BroadcastTransaction(const std::string& hash, const std::string& sender, 
                                           const std::string& receiver, double amt)
    : tx_hash(hash), sender_address(sender), receiver_address(receiver),
      amount(amt), timestamp(static_cast<double>(time(nullptr))),
      is_confirmed(false) {}

// WitnessRecord实现
WitnessRecord::WitnessRecord() : witness_timestamp(0.0) {}

WitnessRecord::WitnessRecord(const std::string& hash) 
    : tx_hash(hash), witness_timestamp(static_cast<double>(time(nullptr))) {}

// UserCredit实现
UserCredit::UserCredit() : registration_timestamp(0.0), is_active(false), 
                           required_witness_count(0), credit_score(0.0) {}

UserCredit::UserCredit(const std::string& addr, int required_count)
    : address(addr), registration_timestamp(static_cast<double>(time(nullptr))),
      is_active(false), required_witness_count(required_count), credit_score(0.0) {}

// CreditManager实现
CreditManager::CreditManager(int min_witness) : min_witness_required(min_witness) {}

void CreditManager::update_user_credit_score(const std::string& address) {
    auto it = user_credits.find(address);
    if (it == user_credits.end()) return;
    
    UserCredit& credit = it->second;
    if (credit.required_witness_count == 0) {
        credit.credit_score = 100.0;
        return;
    }
    
    double progress = static_cast<double>(credit.witnessed_transactions.size()) / 
                     static_cast<double>(credit.required_witness_count);
    credit.credit_score = 50.0 + (progress * 50.0);
    
    if (credit.witnessed_transactions.size() >= credit.required_witness_count) {
        credit.credit_score = 100.0;
    }
}

bool CreditManager::check_witness_completion(const std::string& address) {
    auto it = user_credits.find(address);
    if (it == user_credits.end()) return false;
    return it->second.witnessed_transactions.size() >= it->second.required_witness_count;
}

bool CreditManager::register_user(const std::string& address) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (user_credits.find(address) != user_credits.end()) {
        return false;
    }
    
    int required_count = min_witness_required;
    if (required_count == 0) {
        required_count = static_cast<int>(broadcast_transactions.size());
    }
    
    UserCredit new_user(address, required_count);
    user_credits[address] = new_user;
    
    if (required_count == 0) {
        user_credits[address].is_active = true;
        active_users.insert(address);
        user_credits[address].credit_score = 100.0;
    }
    
    return true;
}

bool CreditManager::broadcast_transaction(const std::string& tx_hash, 
                                          const std::string& sender,
                                          const std::string& receiver, 
                                          double amount) {
    std::lock_guard<std::mutex> lock(mtx);
    
    if (broadcast_transactions.find(tx_hash) != broadcast_transactions.end()) {
        return false;
    }
    
    BroadcastTransaction broadcast_tx(tx_hash, sender, receiver, amount);
    broadcast_transactions[tx_hash] = broadcast_tx;
    pending_witness_queue.push(tx_hash);
    
    return true;
}

bool CreditManager::witness_transaction(const std::string& address, const std::string& tx_hash) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto user_it = user_credits.find(address);
    if (user_it == user_credits.end()) return false;
    if (user_it->second.is_active) return true;
    
    if (broadcast_transactions.find(tx_hash) == broadcast_transactions.end()) return false;
    if (user_it->second.witnessed_transactions.find(tx_hash) != 
        user_it->second.witnessed_transactions.end()) return false;
    
    WitnessRecord record(tx_hash);
    user_it->second.witnessed_transactions.insert(tx_hash);
    user_it->second.witness_history.push_back(record);
    
    update_user_credit_score(address);
    
    if (check_witness_completion(address)) {
        user_it->second.is_active = true;
        active_users.insert(address);
    }
    
    return true;
}

int CreditManager::batch_witness_transactions(const std::string& address, 
                                               const std::vector<std::string>& tx_hashes) {
    int success_count = 0;
    for (const auto& tx_hash : tx_hashes) {
        if (witness_transaction(address, tx_hash)) {
            success_count++;
        }
    }
    return success_count;
}

bool CreditManager::can_user_trade(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = user_credits.find(address);
    if (it == user_credits.end()) return false;
    return it->second.is_active;
}

const UserCredit* CreditManager::get_user_credit(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = user_credits.find(address);
    if (it != user_credits.end()) return &(it->second);
    return nullptr;
}

std::pair<int, int> CreditManager::get_witness_progress(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = user_credits.find(address);
    if (it == user_credits.end()) return {0, 0};
    return {static_cast<int>(it->second.witnessed_transactions.size()), 
            it->second.required_witness_count};
}

std::vector<std::string> CreditManager::get_pending_witness_transactions(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> pending;
    
    if (address.empty()) {
        for (const auto& pair : broadcast_transactions) {
            pending.push_back(pair.first);
        }
    } else {
        auto user_it = user_credits.find(address);
        if (user_it != user_credits.end()) {
            for (const auto& pair : broadcast_transactions) {
                if (user_it->second.witnessed_transactions.find(pair.first) == 
                    user_it->second.witnessed_transactions.end()) {
                    pending.push_back(pair.first);
                }
            }
        }
    }
    
    return pending;
}

bool CreditManager::confirm_transaction(const std::string& tx_hash, const std::string& block_hash) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = broadcast_transactions.find(tx_hash);
    if (it == broadcast_transactions.end()) return false;
    it->second.is_confirmed = true;
    it->second.block_hash = block_hash;
    return true;
}

std::unordered_set<std::string> CreditManager::get_active_users() const {
    std::lock_guard<std::mutex> lock(mtx);
    return active_users;
}

size_t CreditManager::get_total_user_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return user_credits.size();
}

size_t CreditManager::get_active_user_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return active_users.size();
}

size_t CreditManager::get_broadcast_transaction_count() const {
    std::lock_guard<std::mutex> lock(mtx);
    return broadcast_transactions.size();
}