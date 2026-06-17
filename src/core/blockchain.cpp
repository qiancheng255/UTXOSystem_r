/**
 * 区块链模块实现文件
 * 实现UTXO区块链系统
 */

#include "blockchain.h"
#include "crypto.h"
#include <ctime>
#include <iomanip>

Blockchain::Blockchain(int diff) : difficulty(diff), genesis_reward(10.0), savings_pool(2000.0) {
    create_genesis_block();
}

void Blockchain::create_genesis_block() {
    Transaction genesis_tx;
    genesis_tx.tx_hash = "genesis_tx";
    genesis_tx.outputs.push_back({genesis_reward, "GENESIS_POOL"});
    genesis_tx.outputs.push_back({savings_pool, "SAVINGS_POOL"});
    
    for (size_t i = 0; i < genesis_tx.outputs.size(); i++) {
        UTXO utxo{genesis_tx.tx_hash, static_cast<int>(i), 
                  genesis_tx.outputs[i].amount, genesis_tx.outputs[i].address};
        utxo_pool.add_utxo(utxo);
    }
    
    std::vector<Transaction> genesis_txs = {genesis_tx};
    Block genesis_block(std::string(64, '0'), genesis_txs);
    genesis_block.mine_block(difficulty);
    chain.push_back(genesis_block);
    
    std::cout << "Genesis block created!" << std::endl;
    std::cout << "Genesis reward: " << genesis_reward << " units" << std::endl;
    std::cout << "Savings pool: " << savings_pool << " units" << std::endl;
}

bool Blockchain::validate_transaction(const Transaction& tx, 
                                     const std::unordered_set<UTXO, UTXO::Hash>& sender_utxos) {
    double input_sum = 0.0;
    
    for (const auto& tx_input : tx.inputs) {
        bool utxo_exists = false;
        for (const auto& utxo : sender_utxos) {
            if (utxo.tx_hash == tx_input.tx_hash && 
                utxo.output_index == tx_input.output_index) {
                input_sum += utxo.amount;
                utxo_exists = true;
                break;
            }
        }
        if (!utxo_exists) {
            std::cout << "Error: UTXO does not exist" << std::endl;
            return false;
        }
    }
    
    double output_sum = 0.0;
    for (const auto& output : tx.outputs) {
        output_sum += output.amount;
        if (output.amount <= 0) {
            std::cout << "Error: Output amount must be positive" << std::endl;
            return false;
        }
    }
    
    if (std::abs(input_sum - output_sum) > 0.0001) {
        std::cout << "Error: Input amount does not equal output amount" << std::endl;
        return false;
    }
    
    return true;
}

void Blockchain::update_utxo_pool(const std::vector<Transaction>& transactions) {
    for (const auto& tx : transactions) {
        for (const auto& tx_input : tx.inputs) {
            auto all_utxos = utxo_pool.get_all_utxos();
            for (const auto& pair : all_utxos) {
                for (const auto& utxo : pair.second) {
                    if (utxo.tx_hash == tx_input.tx_hash && 
                        utxo.output_index == tx_input.output_index) {
                        utxo_pool.remove_utxo(utxo);
                        goto next_input;
                    }
                }
            }
            next_input:;
        }
        
        for (size_t i = 0; i < tx.outputs.size(); i++) {
            UTXO new_utxo{tx.tx_hash, static_cast<int>(i), 
                         tx.outputs[i].amount, tx.outputs[i].address};
            utxo_pool.add_utxo(new_utxo);
        }
    }
}

Transaction* Blockchain::create_transaction(const std::string& sender_address,
                                           const std::string& recipient_address,
                                           double amount, const Wallet& wallet) {
    auto sender_utxos = utxo_pool.get_utxos_for_address(sender_address);
    double sender_balance = 0.0;
    for (const auto& utxo : sender_utxos) {
        sender_balance += utxo.amount;
    }
    
    if (sender_balance < amount) {
        std::cout << "错误: 余额不足。当前余额: " << sender_balance 
                 << ", 需要: " << amount << std::endl;
        return nullptr;
    }
    
    std::vector<UTXO> selected_utxos;
    double selected_amount = 0.0;
    
    for (const auto& utxo : sender_utxos) {
        if (selected_amount >= amount) break;
        selected_utxos.push_back(utxo);
        selected_amount += utxo.amount;
    }
    
    Transaction tx;
    
    // 发起方根据时间戳生成数字签名
    for (const auto& utxo : selected_utxos) {
        TransactionInput input;
        input.tx_hash = utxo.tx_hash;
        input.output_index = utxo.output_index;
        
        // 生成待签名的数据
        std::string sign_data = utxo.tx_hash + ":" + std::to_string(utxo.output_index);
        
        // 使用钱包根据时间戳生成数字签名（使用哈希函数加密）
        TransactionSignature tx_sig = wallet.sign(sign_data);
        
        // 使用系统公钥加密签名，发送给接收方
        std::string system_public_key = "SYSTEM_PUBLIC_KEY_2024";
        EncryptedSignature enc_sig = wallet.encrypt_signature(tx_sig, recipient_address, system_public_key);
        
        // 将加密签名信息存储在signature字段中
        input.signature = enc_sig.encrypted_data + "|" + 
                         enc_sig.sender_address + "|" + 
                         enc_sig.receiver_address + "|" + 
                         enc_sig.timestamp + "|" + 
                         enc_sig.system_public_key;
        
        std::cout << "数字签名已生成:" << std::endl;
        std::cout << "  签名哈希: " << tx_sig.signature_hash.substr(0, 16) << "..." << std::endl;
        std::cout << "  时间戳: " << tx_sig.timestamp;
        std::cout << "  加密签名已生成，等待接收方验证" << std::endl;
        
        tx.inputs.push_back(input);
    }
    
    tx.outputs.push_back({amount, recipient_address});
    
    double change_amount = selected_amount - amount;
    if (change_amount > 0) {
        tx.outputs.push_back({change_amount, sender_address});
    }
    
    tx.tx_hash = tx.calculate_hash();
    
    if (validate_transaction(tx, sender_utxos)) {
        transaction_pool.push_back(tx);
        std::cout << "交易已创建: " << sender_address << " -> " 
                 << recipient_address << ", 金额: " << amount << std::endl;
        std::cout << "加密数字签名已存储在交易库中" << std::endl;
        std::cout << "等待接收方使用私钥解密验证..." << std::endl;
        return &transaction_pool.back();
    }
    
    std::cout << "交易验证失败" << std::endl;
    return nullptr;
}

void Blockchain::mine_pending_transactions(const std::string& miner_address) {
    if (transaction_pool.empty()) {
        std::cout << "没有待处理的交易" << std::endl;
        return;
    }
    
    Transaction reward_tx;
    reward_tx.outputs.push_back({genesis_reward, miner_address});
    reward_tx.tx_hash = reward_tx.calculate_hash();
    
    std::vector<Transaction> transactions_to_mine;
    transactions_to_mine.push_back(reward_tx);
    transactions_to_mine.insert(transactions_to_mine.end(), 
                               transaction_pool.begin(), transaction_pool.end());
    
    Block new_block(chain.back().hash, transactions_to_mine);
    
    std::cout << "开始挖矿..." << std::endl;
    new_block.mine_block(difficulty);
    std::cout << "Block mined! Hash: " << new_block.hash << std::endl;
    
    update_utxo_pool(transactions_to_mine);
    chain.push_back(new_block);
    transaction_pool.clear();
    
    std::cout << "Miner " << miner_address << " received " 
             << genesis_reward << " units reward" << std::endl;
}

double Blockchain::get_balance(const std::string& address) const {
    return utxo_pool.get_balance(address);
}

void Blockchain::add_special_transaction(const Transaction& tx) {
    for (size_t i = 0; i < tx.outputs.size(); i++) {
        UTXO new_utxo{tx.tx_hash, static_cast<int>(i), 
                     tx.outputs[i].amount, tx.outputs[i].address};
        utxo_pool.add_utxo(new_utxo);
    }
}

void Blockchain::remove_genesis_savings_utxo() {
    auto savings_utxos = utxo_pool.get_utxos_for_address("SAVINGS_POOL");
    for (const auto& utxo : savings_utxos) {
        if (utxo.tx_hash == "genesis_tx") {
            utxo_pool.remove_utxo(utxo);
            break;
        }
    }
}

void Blockchain::print_blockchain_info() const {
    std::cout << "\n==================================================" << std::endl;
    std::cout << "Blockchain Info" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Chain length: " << chain.size() << std::endl;
    std::cout << "Mining difficulty: " << difficulty << std::endl;
    std::cout << "Genesis reward: " << genesis_reward << std::endl;
    std::cout << "Savings pool balance: " << utxo_pool.get_balance("SAVINGS_POOL") << std::endl;
    
    std::cout << "\nAll address balances:" << std::endl;
    const auto& all_utxos = utxo_pool.get_all_utxos();
    for (const auto& pair : all_utxos) {
        double balance = 0.0;
        for (const auto& utxo : pair.second) {
            balance += utxo.amount;
        }
        std::cout << "  " << pair.first << ": " << balance << " units" << std::endl;
    }
    
    std::cout << "\n区块详情:" << std::endl;
    for (size_t i = 0; i < chain.size(); i++) {
        std::cout << "  区块 " << i << ":" << std::endl;
        std::cout << "    哈希: " << chain[i].hash << std::endl;
        std::cout << "    前一个哈希: " << chain[i].previous_hash << std::endl;
        std::cout << "    交易数量: " << chain[i].transactions.size() << std::endl;
        
        time_t time = static_cast<time_t>(chain[i].timestamp);
        char time_str[26];
        ctime_s(time_str, sizeof(time_str), &time);
        std::cout << "    时间戳: " << time_str;
    }
    std::cout << "==================================================" << std::endl;
}

// 获取交易池（用于接收方查询交易信息）
const std::vector<Transaction>& Blockchain::get_transaction_pool() const {
    return transaction_pool;
}

// 获取所有区块
const std::vector<Block>& Blockchain::get_chain() const {
    return chain;
}

// 验证交易签名（接收方验证）
bool Blockchain::verify_transaction_signature(const Transaction& tx) const {
    if (tx.inputs.empty()) {
        std::cout << "交易无输入，跳过签名验证" << std::endl;
        return true;  // 特殊交易（如奖励交易）可能没有输入
    }
    
    for (const auto& input : tx.inputs) {
        // 解析签名数据
        std::string sig_data = input.signature;
        size_t pos1 = sig_data.find('|');
        size_t pos2 = sig_data.find('|', pos1 + 1);
        size_t pos3 = sig_data.find('|', pos2 + 1);
        
        if (pos1 == std::string::npos || pos2 == std::string::npos || pos3 == std::string::npos) {
            std::cout << "签名格式无效" << std::endl;
            return false;
        }
        
        std::string signature_hash = sig_data.substr(0, pos1);
        std::string signer_address = sig_data.substr(pos1 + 1, pos2 - pos1 - 1);
        std::string timestamp = sig_data.substr(pos2 + 1, pos3 - pos2 - 1);
        std::string data_hash = sig_data.substr(pos3 + 1);
        
        // 验证签名哈希不为空
        if (signature_hash.empty()) {
            std::cout << "签名哈希为空" << std::endl;
            return false;
        }
        
        // 验证数据哈希
        std::string expected_data = input.tx_hash + ":" + std::to_string(input.output_index);
        std::string computed_hash = Crypto::sha256(expected_data);
        
        if (computed_hash != data_hash) {
            std::cout << "数据哈希验证失败" << std::endl;
            return false;
        }
        
        std::cout << "签名验证通过: 签名者=" << signer_address << std::endl;
    }
    
    return true;
}

// 接收方确认交易
bool Blockchain::confirm_transaction(const Transaction& tx, const std::string& recipient_address,
                                    const std::string& user_private_key) {
    // 检查交易是否在交易池中
    bool found = false;
    for (const auto& pool_tx : transaction_pool) {
        if (pool_tx.tx_hash == tx.tx_hash) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        std::cout << "交易不在交易池中" << std::endl;
        return false;
    }
    
    // 检查接收方是否是交易的输出地址之一
    bool is_recipient = false;
    for (const auto& output : tx.outputs) {
        if (output.address == recipient_address) {
            is_recipient = true;
            break;
        }
    }
    
    if (!is_recipient) {
        std::cout << "您不是此交易的接收方" << std::endl;
        return false;
    }
    
    // 验证加密签名（使用用户私钥解密）
    std::cout << "\n正在验证加密签名..." << std::endl;
    
    for (const auto& input : tx.inputs) {
        // 解析加密签名数据
        std::string sig_data = input.signature;
        size_t pos1 = sig_data.find('|');
        size_t pos2 = sig_data.find('|', pos1 + 1);
        size_t pos3 = sig_data.find('|', pos2 + 1);
        size_t pos4 = sig_data.find('|', pos3 + 1);
        
        if (pos1 == std::string::npos || pos2 == std::string::npos || 
            pos3 == std::string::npos || pos4 == std::string::npos) {
            std::cout << "加密签名格式无效" << std::endl;
            return false;
        }
        
        std::string encrypted_data = sig_data.substr(0, pos1);
        std::string sender_address = sig_data.substr(pos1 + 1, pos2 - pos1 - 1);
        std::string receiver_address = sig_data.substr(pos2 + 1, pos3 - pos2 - 1);
        std::string timestamp = sig_data.substr(pos3 + 1, pos4 - pos3 - 1);
        std::string system_public_key = sig_data.substr(pos4 + 1);
        
        // 验证接收方地址
        if (receiver_address != recipient_address) {
            std::cout << "加密签名接收方不匹配" << std::endl;
            return false;
        }
        
        // 构造加密签名结构
        EncryptedSignature enc_sig;
        enc_sig.encrypted_data = encrypted_data;
        enc_sig.sender_address = sender_address;
        enc_sig.receiver_address = receiver_address;
        enc_sig.timestamp = timestamp;
        enc_sig.system_public_key = system_public_key;
        
        // 使用用户私钥解密验证
        std::string expected_data = input.tx_hash + ":" + std::to_string(input.output_index);
        if (!Wallet::decrypt_and_verify(enc_sig, user_private_key, expected_data)) {
            std::cout << "加密签名解密验证失败" << std::endl;
            return false;
        }
    }
    
    std::cout << "\n交易确认成功!" << std::endl;
    std::cout << "交易哈希: " << tx.tx_hash << std::endl;
    
    // 显示接收到的金额
    for (const auto& output : tx.outputs) {
        if (output.address == recipient_address) {
            std::cout << "接收金额: " << output.amount << " 单位" << std::endl;
        }
    }
    
    return true;
}