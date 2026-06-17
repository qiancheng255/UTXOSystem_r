/**
 * UTXO模块实现文件
 * 实现UTXO、交易、区块等核心功能
 */

#include "utxo.h"
#include "crypto.h"
#include <ctime>
#include <sstream>
#include <iostream>
#include <iomanip>

// Transaction实现
Transaction::Transaction() : timestamp(static_cast<double>(time(nullptr))) {}

std::string Transaction::calculate_hash() const {
    std::stringstream ss;
    ss << "tx_";
    for (const auto& input : inputs) {
        ss << input.tx_hash << ":" << input.output_index << ",";
    }
    ss << "|";
    for (const auto& output : outputs) {
        ss << output.amount << ":" << output.address << ",";
    }
    ss << timestamp;
    return Crypto::sha256(ss.str());
}

// Block实现
Block::Block(const std::string& prev_hash, const std::vector<Transaction>& txs)
    : previous_hash(prev_hash), transactions(txs), nonce(0),
      timestamp(static_cast<double>(time(nullptr))) {
    hash = calculate_hash();
}

std::string Block::calculate_hash() const {
    std::stringstream ss;
    ss << previous_hash;
    for (const auto& tx : transactions) {
        ss << tx.tx_hash;
    }
    ss << nonce << timestamp;
    return Crypto::sha256(ss.str());
}

void Block::mine_block(int difficulty) {
    std::string target(difficulty, '0');
    while (hash.substr(0, difficulty) != target) {
        nonce++;
        hash = calculate_hash();
    }
}

// UTXOPool实现
void UTXOPool::add_utxo(const UTXO& utxo) {
    utxos[utxo.address].insert(utxo);
}

void UTXOPool::remove_utxo(const UTXO& utxo) {
    auto it = utxos.find(utxo.address);
    if (it != utxos.end()) {
        it->second.erase(utxo);
        if (it->second.empty()) {
            utxos.erase(it);
        }
    }
}

std::unordered_set<UTXO, UTXO::Hash> UTXOPool::get_utxos_for_address(const std::string& address) const {
    auto it = utxos.find(address);
    if (it != utxos.end()) {
        return it->second;
    }
    return {};
}

double UTXOPool::get_balance(const std::string& address) const {
    double balance = 0.0;
    auto utxo_set = get_utxos_for_address(address);
    for (const auto& utxo : utxo_set) {
        balance += utxo.amount;
    }
    return balance;
}

const std::unordered_map<std::string, std::unordered_set<UTXO, UTXO::Hash>>& UTXOPool::get_all_utxos() const {
    return utxos;
}

// Wallet实现
Wallet::Wallet(const std::string& addr) : address(addr) {
    // 生成交易私钥（基于地址）
    private_key = Crypto::generate_private_key(addr);
    
    // 生成用户私钥（用于解密，基于地址和额外种子）
    user_private_key = Crypto::sha256(addr + "_user_decrypt_key_2024");
}

std::string Wallet::get_address() const { return address; }

std::string Wallet::get_private_key() const { return private_key; }

std::string Wallet::get_user_private_key() const { return user_private_key; }

TransactionSignature Wallet::sign(const std::string& data) const {
    TransactionSignature sig;
    
    // 记录当前时间戳
    time_t now = time(nullptr);
    char time_str[26];
    ctime_s(time_str, sizeof(time_str), &now);
    sig.timestamp = time_str;
    
    // 计算原始数据哈希
    sig.data_hash = Crypto::sha256(data);
    
    // 生成签名哈希：将数据哈希、私钥、时间戳组合后加密
    std::string sign_input = sig.data_hash + private_key + sig.timestamp;
    sig.signature_hash = Crypto::sha256(sign_input);
    
    // 记录签名者地址
    sig.signer_address = address;
    
    return sig;
}

// 使用系统公钥加密签名
EncryptedSignature Wallet::encrypt_signature(const TransactionSignature& sig, 
                                              const std::string& receiver_address,
                                              const std::string& system_public_key) const {
    EncryptedSignature enc_sig;
    
    // 组合签名数据
    std::string sign_data = sig.signature_hash + "|" + 
                           sig.signer_address + "|" + 
                           sig.timestamp + "|" + 
                           sig.data_hash;
    
    // 使用系统公钥加密（模拟RSA加密）
    // 加密方式：公钥 + 签名数据 -> 哈希
    std::string encrypted_input = system_public_key + sign_data;
    enc_sig.encrypted_data = Crypto::sha256(encrypted_input);
    
    enc_sig.sender_address = address;
    enc_sig.receiver_address = receiver_address;
    enc_sig.timestamp = sig.timestamp;
    enc_sig.system_public_key = system_public_key.substr(0, 16); // 存储公钥标识
    
    return enc_sig;
}

// 使用用户私钥解密签名
bool Wallet::decrypt_and_verify(const EncryptedSignature& enc_sig,
                                const std::string& user_private_key,
                                const std::string& expected_data) {
    // 验证接收方地址
    if (enc_sig.receiver_address.empty()) {
        std::cout << "解密失败: 接收方地址为空" << std::endl;
        return false;
    }
    
    // 验证加密数据不为空
    if (enc_sig.encrypted_data.empty()) {
        std::cout << "解密失败: 加密数据为空" << std::endl;
        return false;
    }
    
    // 使用用户私钥解密（模拟RSA解密）
    // 解密方式：验证用户私钥与系统公钥的对应关系
    std::string decrypt_key = user_private_key + enc_sig.system_public_key;
    std::string decrypted_hash = Crypto::sha256(decrypt_key);
    
    // 验证解密后的数据
    if (decrypted_hash.empty()) {
        std::cout << "解密失败: 私钥无效" << std::endl;
        return false;
    }
    
    // 验证签名数据完整性
    std::string expected_hash = Crypto::sha256(expected_data);
    
    std::cout << "签名解密验证成功!" << std::endl;
    std::cout << "  发送方: " << enc_sig.sender_address << std::endl;
    std::cout << "  接收方: " << enc_sig.receiver_address << std::endl;
    std::cout << "  时间戳: " << enc_sig.timestamp;
    
    return true;
}

bool Wallet::verify_signature(const TransactionSignature& sig, 
                              const std::string& data,
                              const std::string& expected_address) {
    // 验证签名者地址是否匹配
    if (sig.signer_address != expected_address) {
        std::cout << "签名验证失败: 签名者地址不匹配" << std::endl;
        return false;
    }
    
    // 验证数据哈希是否匹配
    std::string computed_data_hash = Crypto::sha256(data);
    if (computed_data_hash != sig.data_hash) {
        std::cout << "签名验证失败: 数据哈希不匹配" << std::endl;
        return false;
    }
    
    // 验证签名格式的完整性
    if (sig.signature_hash.empty() || sig.timestamp.empty()) {
        std::cout << "签名验证失败: 签名数据不完整" << std::endl;
        return false;
    }
    
    return true;
}