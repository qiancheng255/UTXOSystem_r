/**
 * 区块链模块头文件
 * 实现UTXO区块链系统
 */

#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "utxo.h"
#include <vector>
#include <iostream>
#include <cmath>

// 区块链
class Blockchain {
private:
    int difficulty;
    std::vector<Block> chain;
    UTXOPool utxo_pool;
    std::vector<Transaction> transaction_pool;
    double genesis_reward;
    double savings_pool;
    
    void create_genesis_block();
    bool validate_transaction(const Transaction& tx, 
                             const std::unordered_set<UTXO, UTXO::Hash>& sender_utxos);
    void update_utxo_pool(const std::vector<Transaction>& transactions);
    
public:
    Blockchain(int diff = 2);
    
    Transaction* create_transaction(const std::string& sender_address,
                                   const std::string& recipient_address,
                                   double amount, const Wallet& wallet);
    
    void mine_pending_transactions(const std::string& miner_address);
    double get_balance(const std::string& address) const;
    void add_special_transaction(const Transaction& tx);
    void remove_genesis_savings_utxo();
    void print_blockchain_info() const;
    
    // 获取交易池（用于接收方查询交易信息）
    const std::vector<Transaction>& get_transaction_pool() const;
    
    // 获取所有区块
    const std::vector<Block>& get_chain() const;
    
    // 验证交易签名（接收方验证）
    bool verify_transaction_signature(const Transaction& tx) const;
    
    // 接收方确认交易（使用私钥解密验证）
    bool confirm_transaction(const Transaction& tx, const std::string& recipient_address,
                            const std::string& user_private_key);
};

#endif // BLOCKCHAIN_H