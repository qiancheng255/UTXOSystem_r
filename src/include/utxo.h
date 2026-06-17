/**
 * UTXO模块头文件
 * 定义UTXO、交易、区块等核心数据结构
 */

#ifndef UTXO_H
#define UTXO_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>

// UTXO结构
struct UTXO {
    std::string tx_hash;
    int output_index;
    double amount;
    std::string address;
    
    bool operator==(const UTXO& other) const {
        return tx_hash == other.tx_hash && output_index == other.output_index;
    }
    
    struct Hash {
        size_t operator()(const UTXO& utxo) const {
            return std::hash<std::string>()(utxo.tx_hash) ^ 
                   (std::hash<int>()(utxo.output_index) << 1);
        }
    };
};

// 交易输入
struct TransactionInput {
    std::string tx_hash;
    int output_index;
    std::string signature;
};

// 交易输出
struct TransactionOutput {
    double amount;
    std::string address;
};

// 交易
struct Transaction {
    std::string tx_hash;
    std::vector<TransactionInput> inputs;
    std::vector<TransactionOutput> outputs;
    double timestamp;
    
    Transaction();
    std::string calculate_hash() const;
};

// 区块
struct Block {
    std::string previous_hash;
    std::vector<Transaction> transactions;
    int nonce;
    double timestamp;
    std::string hash;
    
    Block(const std::string& prev_hash, const std::vector<Transaction>& txs);
    std::string calculate_hash() const;
    void mine_block(int difficulty);
};

// UTXO池
class UTXOPool {
private:
    std::unordered_map<std::string, std::unordered_set<UTXO, UTXO::Hash>> utxos;
    
public:
    void add_utxo(const UTXO& utxo);
    void remove_utxo(const UTXO& utxo);
    std::unordered_set<UTXO, UTXO::Hash> get_utxos_for_address(const std::string& address) const;
    double get_balance(const std::string& address) const;
    const std::unordered_map<std::string, std::unordered_set<UTXO, UTXO::Hash>>& get_all_utxos() const;
};

// 交易数字签名结构
struct TransactionSignature {
    std::string signature_hash;  // 签名哈希（使用SHA256加密）
    std::string signer_address;  // 签名者地址
    std::string timestamp;       // 签名时间戳
    std::string data_hash;       // 原始数据哈希
};

// 加密签名结构（用于接收方验证）
struct EncryptedSignature {
    std::string encrypted_data;    // 加密后的签名数据
    std::string sender_address;    // 发送方地址
    std::string receiver_address;  // 接收方地址
    std::string timestamp;         // 时间戳
    std::string system_public_key; // 系统公钥标识
};

// 系统密钥对（公钥在系统内，私钥分发给用户）
struct SystemKeyPair {
    std::string public_key;   // 系统公钥（用于加密）
    std::string private_key;  // 系统私钥（用于解密）
};

// 钱包
class Wallet {
private:
    std::string address;
    std::string private_key;
    std::string user_private_key;  // 用户私钥（用于解密）
    
public:
    Wallet(const std::string& addr);
    std::string get_address() const;
    std::string get_private_key() const;
    std::string get_user_private_key() const;
    
    TransactionSignature sign(const std::string& data) const;
    
    // 使用系统公钥加密签名
    EncryptedSignature encrypt_signature(const TransactionSignature& sig, 
                                         const std::string& receiver_address,
                                         const std::string& system_public_key) const;
    
    // 使用用户私钥解密签名
    static bool decrypt_and_verify(const EncryptedSignature& enc_sig,
                                   const std::string& user_private_key,
                                   const std::string& expected_data);
    
    static bool verify_signature(const TransactionSignature& sig, 
                                 const std::string& data,
                                 const std::string& expected_address);
};

#endif // UTXO_H