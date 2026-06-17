/**
 * UTXO 安全电子货币交易系统 - 主程序
 * 
 * 集成功能:
 * - UTXO 区块链系统
 * - 用户信用体系 (见证机制)
 * - 用户数据库 (60人分组管理)
 * - 生产环境加密 (Windows BCrypt API)
 * - 用户/管理员身份认证系统
 * - 待交易数据库
 * - 货币记录RSA加密数据库
 */

// Windows API 头文件 (必须在其他头文件之前)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <iostream>
#include <string>
#include <vector>
#include <limits>

#include "crypto.h"
#include "utxo.h"
#include "blockchain.h"
#include "credit_system.h"
#include "user_database.h"
#include "user_auth.h"
#include "pending_transaction.h"
#include "currency_record.h"
#include "privacy_utils.h"

// 清空输入缓冲区
void clear_input() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// 打印分隔线
void print_separator() {
    std::cout << "==================================================" << std::endl;
}

// 打印标题
void print_title(const std::string& title) {
    std::cout << std::endl;
    print_separator();
    std::cout << title << std::endl;
    print_separator();
    std::cout << std::endl;
}

// 用户菜单
void user_menu(AuthManager& auth, Blockchain& blockchain, 
               CreditManager& credit_manager, UserDatabase& user_db,
               PendingTransactionDB& pending_tx_db,
               CurrencyRecordDB& currency_db,
               const std::string& current_user) {
    int choice = 0;
    
    while (choice != 8) {
        print_title("用户菜单 - " + current_user);
        std::cout << "1. 发起交易" << std::endl;
        std::cout << "2. 查询交易记录" << std::endl;
        std::cout << "3. 查看个人信息" << std::endl;
        std::cout << "4. 查看余额" << std::endl;
        std::cout << "5. 待确认交易" << std::endl;
        std::cout << "6. 查看货币加密记录" << std::endl;
        std::cout << "7. 查看匿名地址" << std::endl;
        std::cout << "8. 退出登录" << std::endl;
        std::cout << std::endl;
        std::cout << "请选择操作: ";
        
        std::cin >> choice;
        clear_input();
        
        switch (choice) {
            case 1: {
                print_title("发起交易 (匿名地址保护)");
                std::string receiver;
                double amount;
                
                std::cout << "收款方用户名: ";
                std::getline(std::cin, receiver);
                
                std::cout << "交易金额: ";
                std::cin >> amount;
                clear_input();
                
                if (amount <= 0) {
                    std::cout << "错误: 交易金额必须为正数" << std::endl;
                    break;
                }
                
                const UserAccount* sender_account = auth.get_account(current_user);
                const UserAccount* receiver_account = auth.get_account(receiver);
                
                if (!sender_account || !receiver_account) {
                    std::cout << "错误: 用户不存在" << std::endl;
                    break;
                }
                
                if (sender_account->status != UserStatus::ACTIVE) {
                    std::cout << "错误: 您的账户状态异常" << std::endl;
                    break;
                }
                
                // 生成交易哈希
                std::string tx_hash = "tx_" + std::to_string(time(nullptr));
                
                // 为本次交易生成新的匿名地址
                std::string sender_anon = PrivacyUtils::generate_transaction_anonymous_address(
                    sender_account->address, tx_hash);
                std::string receiver_anon = PrivacyUtils::generate_transaction_anonymous_address(
                    receiver_account->address, tx_hash);
                
                std::cout << "\n--- 匿名地址信息 ---" << std::endl;
                std::cout << "发送方匿名地址: " << sender_anon << std::endl;
                std::cout << "接收方匿名地址: " << receiver_anon << std::endl;
                
                Wallet sender_wallet(sender_account->address);
                Transaction* tx = blockchain.create_transaction(
                    sender_account->address, receiver_account->address, amount, sender_wallet
                );
                
                if (tx) {
                    credit_manager.broadcast_transaction(tx_hash, current_user, receiver, amount);
                    user_db.record_transaction(tx_hash, sender_account->address, 
                                              receiver_account->address, amount);
                    
                    // 记录到待交易数据库
                    PendingTransactionRecord pending_record;
                    pending_record.tx_hash = tx_hash;
                    pending_record.sender_address = sender_account->address;
                    pending_record.sender_username = current_user;
                    pending_record.receiver_address = receiver_account->address;
                    pending_record.receiver_username = receiver;
                    pending_record.amount = amount;
                    pending_record.timestamp = static_cast<double>(time(nullptr));
                    pending_record.signature_status = "待验证";
                    pending_record.verification_key = sender_wallet.get_user_private_key();
                    
                    pending_tx_db.add_pending_transaction(pending_record);
                    
                    // RSA加密并记录到货币记录数据库
                    std::string record_id = currency_db.record_currency_transfer(
                        sender_account->address, receiver_account->address, amount, tx_hash);
                    
                    std::cout << "\n交易创建成功!" << std::endl;
                    std::cout << "交易哈希: " << tx_hash << std::endl;
                    std::cout << "加密签名已存储到待交易数据库" << std::endl;
                    std::cout << "RSA加密记录ID: " << record_id << std::endl;
                    std::cout << "等待收款方 " << receiver << " 使用私钥验证..." << std::endl;
                } else {
                    std::cout << "交易创建失败" << std::endl;
                }
                
                break;
            }
            case 2: {
                print_title("查询交易记录");
                const UserAccount* account = auth.get_account(current_user);
                if (!account) {
                    std::cout << "错误: 无法获取账户信息" << std::endl;
                    break;
                }
                
                std::cout << auth.get_transaction_history(current_user) << std::endl;
                
                // 显示待处理交易
                const auto& tx_pool = blockchain.get_transaction_pool();
                if (!tx_pool.empty()) {
                    std::cout << "\n待处理交易:" << std::endl;
                    for (const auto& tx : tx_pool) {
                        std::cout << "交易哈希: " << tx.tx_hash << std::endl;
                        for (const auto& output : tx.outputs) {
                            if (output.address == account->address) {
                                std::cout << "  收款金额: " << output.amount << std::endl;
                                std::cout << "  发送方: ";
                                for (const auto& input : tx.inputs) {
                                    std::cout << input.tx_hash << " ";
                                }
                                std::cout << std::endl;
                            }
                        }
                    }
                }
                
                break;
            }
            case 3: {
                print_title("个人信息");
                const UserAccount* account = auth.get_account(current_user);
                if (account) {
                    std::cout << "用户名: " << account->username << std::endl;
                    std::cout << "钱包地址: " << account->address << std::endl;
                    std::cout << "角色: " << (account->role == UserRole::ADMIN ? "管理员" : "普通用户") << std::endl;
                    std::cout << "状态: " << (account->status == UserStatus::ACTIVE ? "正常" : "异常") << std::endl;
                    
                    time_t reg_time = static_cast<time_t>(account->registration_time);
                    char time_str[26];
                    ctime_s(time_str, sizeof(time_str), &reg_time);
                    std::cout << "注册时间: " << time_str;
                    
                    Wallet wallet(account->address);
                    std::cout << "公钥标识: " << wallet.get_user_private_key().substr(0, 16) << "..." << std::endl;
                    
                    // 显示货币记录信息
                    UserBalanceRecord balance_record = currency_db.get_user_balance(account->address);
                    std::cout << "\n--- 货币记录信息 ---" << std::endl;
                    std::cout << "记录总余额: " << balance_record.total_balance << " 单位" << std::endl;
                    std::cout << "冻结余额: " << balance_record.frozen_balance << " 单位" << std::endl;
                    std::cout << "总交易数: " << balance_record.total_transactions << std::endl;
                }
                break;
            }
            case 4: {
                print_title("查看余额");
                const UserAccount* account = auth.get_account(current_user);
                if (account) {
                    double balance = blockchain.get_balance(account->address);
                    std::cout << "区块链余额: " << balance << " 单位" << std::endl;
                    
                    // 显示货币记录余额
                    UserBalanceRecord balance_record = currency_db.get_user_balance(account->address);
                    std::cout << "记录余额: " << balance_record.total_balance << " 单位" << std::endl;
                }
                break;
            }
            case 5: {
                print_title("待确认交易");
                const UserAccount* account = auth.get_account(current_user);
                if (!account) {
                    std::cout << "错误: 无法获取账户信息" << std::endl;
                    break;
                }
                
                // 获取待接收交易
                auto pending_received = pending_tx_db.get_pending_received_transactions(current_user);
                
                if (pending_received.empty()) {
                    std::cout << "暂无待接收的交易" << std::endl;
                } else {
                    std::cout << "待接收的交易:" << std::endl;
                    std::cout << "--------------------------------------------------" << std::endl;
                    
                    for (const auto& record : pending_received) {
                        std::cout << "\n交易哈希: " << record.tx_hash << std::endl;
                        std::cout << "发送方: " << record.sender_username << " (" << record.sender_address << ")" << std::endl;
                        std::cout << "金额: " << record.amount << " 单位" << std::endl;
                        std::cout << "签名状态: " << record.signature_status << std::endl;
                        
                        time_t tx_time = static_cast<time_t>(record.timestamp);
                        char time_str[26];
                        ctime_s(time_str, sizeof(time_str), &tx_time);
                        std::cout << "交易时间: " << time_str;
                        
                        // 确认交易选项
                        std::cout << "\n是否使用私钥解密并确认交易? (y/n): ";
                        char confirm;
                        std::cin >> confirm;
                        clear_input();
                        
                        if (confirm == 'y' || confirm == 'Y') {
                            // 获取用户私钥进行解密
                            Wallet user_wallet(account->address);
                            std::string user_private_key = user_wallet.get_user_private_key();
                            
                            std::cout << "\n正在使用私钥解密签名..." << std::endl;
                            
                            // 验证签名并确认交易
                            bool verified = false;
                            for (const auto& tx : blockchain.get_transaction_pool()) {
                                if (tx.tx_hash == record.tx_hash) {
                                    if (blockchain.confirm_transaction(tx, account->address, user_private_key)) {
                                        verified = true;
                                        
                                        // 更新状态为已验证
                                        pending_tx_db.update_signature_status(record.tx_hash, "已验证");
                                        
                                        // 自动挖矿确认
                                        blockchain.mine_pending_transactions("MINER_ADDRESS");
                                        
                                        // 移除已完成的交易
                                        pending_tx_db.remove_completed_transaction(record.tx_hash);
                                        
                                        // 更新信用系统和用户数据库
                                        credit_manager.confirm_transaction(record.tx_hash, "block_hash");
                                        user_db.confirm_transaction(record.tx_hash, "block_hash");
                                        
                                        std::cout << "交易已确认并记录到区块链!" << std::endl;
                                    }
                                    break;
                                }
                            }
                            
                            if (!verified) {
                                std::cout << "交易验证失败" << std::endl;
                            }
                        } else {
                            std::cout << "已跳过该交易" << std::endl;
                        }
                    }
                }
                
                // 显示已发送的待确认交易
                auto pending_sent = pending_tx_db.get_pending_sent_transactions(current_user);
                if (!pending_sent.empty()) {
                    std::cout << "\n您发送的待确认交易:" << std::endl;
                    std::cout << "--------------------------------------------------" << std::endl;
                    
                    for (const auto& record : pending_sent) {
                        std::cout << "\n交易哈希: " << record.tx_hash << std::endl;
                        std::cout << "收款方: " << record.receiver_username << std::endl;
                        std::cout << "金额: " << record.amount << " 单位" << std::endl;
                        std::cout << "签名状态: " << record.signature_status << std::endl;
                    }
                }
                
                break;
            }
            case 6: {
                print_title("货币加密记录");
                const UserAccount* account = auth.get_account(current_user);
                if (!account) {
                    std::cout << "错误: 无法获取账户信息" << std::endl;
                    break;
                }
                
                // 导出用户货币报告
                std::string report = currency_db.export_user_currency_report(account->address);
                std::cout << report << std::endl;
                
                // 显示系统公钥信息
                std::cout << "\n--- 系统RSA密钥信息 ---" << std::endl;
                std::cout << "系统密钥ID: " << currency_db.get_system_key_id() << std::endl;
                std::cout << "系统公钥: " << currency_db.get_system_public_key().substr(0, 32) << "..." << std::endl;
                
                break;
            }
            case 7: {
                print_title("匿名地址信息");
                const UserAccount* account = auth.get_account(current_user);
                if (!account) {
                    std::cout << "错误: 无法获取账户信息" << std::endl;
                    break;
                }
                
                // 获取用户的匿名地址
                std::string anon_addr = PrivacyUtils::get_anonymous_address(account->address);
                
                std::cout << "您的真实地址: " << account->address << std::endl;
                std::cout << "您的匿名地址: " << (anon_addr.empty() ? "暂无" : anon_addr) << std::endl;
                
                std::cout << "\n--- 匿名地址说明 ---" << std::endl;
                std::cout << "1. 每次交易会自动生成新的匿名地址" << std::endl;
                std::cout << "2. 匿名地址由时间戳和哈希值随机拼合生成" << std::endl;
                std::cout << "3. 交易接收方无法通过匿名地址定位发送方" << std::endl;
                std::cout << "4. 匿名地址格式: anon_ + 40位十六进制字符" << std::endl;
                
                // 显示用户的所有匿名地址
                auto all_mappings = PrivacyUtils::get_all_mappings();
                std::cout << "\n--- 您的所有匿名地址 ---" << std::endl;
                int count = 0;
                for (const auto& pair : all_mappings) {
                    if (pair.first == account->address || pair.second == account->address) {
                        count++;
                        std::cout << count << ". " << pair.second << std::endl;
                    }
                }
                
                if (count == 0) {
                    std::cout << "暂无匿名地址记录" << std::endl;
                }
                
                break;
            }
            case 8: {
                std::cout << "已退出登录" << std::endl;
                break;
            }
            default: {
                std::cout << "无效选项，请重新选择" << std::endl;
                break;
            }
        }
    }
}

// 管理员菜单
void admin_menu(AuthManager& auth, Blockchain& blockchain, 
                CreditManager& credit_manager, UserDatabase& user_db,
                CurrencyRecordDB& currency_db,
                const std::string& current_admin) {
    int choice = 0;
    
    while (choice != 10) {
        print_title("管理员菜单 - " + current_admin);
        std::cout << "1. 查看所有用户" << std::endl;
        std::cout << "2. 查询用户信息" << std::endl;
        std::cout << "3. 查看所有交易" << std::endl;
        std::cout << "4. 删除用户" << std::endl;
        std::cout << "5. 加入黑名单" << std::endl;
        std::cout << "6. 移出黑名单" << std::endl;
        std::cout << "7. 查看黑名单" << std::endl;
        std::cout << "8. 货币系统统计" << std::endl;
        std::cout << "9. 查看匿名地址映射" << std::endl;
        std::cout << "10. 退出登录" << std::endl;
        std::cout << std::endl;
        std::cout << "请选择操作: ";
        
        std::cin >> choice;
        clear_input();
        
        switch (choice) {
            case 1: {
                print_title("所有用户");
                std::cout << auth.get_all_users_info() << std::endl;
                break;
            }
            case 2: {
                print_title("查询用户信息");
                std::string username;
                std::cout << "请输入用户名: ";
                std::getline(std::cin, username);
                
                const UserAccount* account = auth.get_account(username);
                if (account) {
                    std::cout << "用户名: " << account->username << std::endl;
                    std::cout << "钱包地址: " << account->address << std::endl;
                    std::cout << "角色: " << (account->role == UserRole::ADMIN ? "管理员" : "普通用户") << std::endl;
                    std::cout << "状态: " << (account->status == UserStatus::ACTIVE ? "正常" : 
                              (account->status == UserStatus::SUSPENDED ? "已停用" : "已拉黑")) << std::endl;
                    
                    time_t reg_time = static_cast<time_t>(account->registration_time);
                    char time_str[26];
                    ctime_s(time_str, sizeof(time_str), &reg_time);
                    std::cout << "注册时间: " << time_str;
                    
                    double balance = blockchain.get_balance(account->address);
                    std::cout << "区块链余额: " << balance << " 单位" << std::endl;
                    
                    // 显示货币记录余额
                    UserBalanceRecord balance_record = currency_db.get_user_balance(account->address);
                    std::cout << "记录余额: " << balance_record.total_balance << " 单位" << std::endl;
                    std::cout << "总交易数: " << balance_record.total_transactions << std::endl;
                } else {
                    std::cout << "用户不存在" << std::endl;
                }
                break;
            }
            case 3: {
                print_title("所有交易");
                const auto& chain = blockchain.get_chain();
                for (const auto& block : chain) {
                    std::cout << "\n区块哈希: " << block.hash << std::endl;
                    std::cout << "前一区块哈希: " << block.previous_hash << std::endl;
                    std::cout << "交易数量: " << block.transactions.size() << std::endl;
                    
                    for (const auto& tx : block.transactions) {
                        std::cout << "  交易: " << tx.tx_hash << std::endl;
                        for (const auto& output : tx.outputs) {
                            std::cout << "    -> " << output.amount << " 转至 " << output.address << std::endl;
                        }
                    }
                }
                
                const auto& tx_pool = blockchain.get_transaction_pool();
                if (!tx_pool.empty()) {
                    std::cout << "\n待处理交易:" << std::endl;
                    for (const auto& tx : tx_pool) {
                        std::cout << "  交易: " << tx.tx_hash << std::endl;
                        for (const auto& output : tx.outputs) {
                            std::cout << "    -> " << output.amount << " 转至 " << output.address << std::endl;
                        }
                    }
                }
                break;
            }
            case 4: {
                print_title("删除用户");
                std::string username;
                std::cout << "请输入要删除的用户名: ";
                std::getline(std::cin, username);
                
                if (auth.is_admin(username)) {
                    std::cout << "错误: 不能删除管理员账户" << std::endl;
                } else if (auth.remove_user(current_admin, username)) {
                    std::cout << "用户 " << username << " 已删除" << std::endl;
                } else {
                    std::cout << "删除失败" << std::endl;
                }
                break;
            }
            case 5: {
                print_title("加入黑名单");
                std::string username, reason;
                std::cout << "请输入用户名: ";
                std::getline(std::cin, username);
                std::cout << "请输入原因: ";
                std::getline(std::cin, reason);
                
                if (auth.is_admin(username)) {
                    std::cout << "错误: 不能将管理员加入黑名单" << std::endl;
                } else if (auth.blacklist_user(current_admin, username, reason)) {
                    std::cout << "用户 " << username << " 已加入黑名单" << std::endl;
                } else {
                    std::cout << "操作失败" << std::endl;
                }
                break;
            }
            case 6: {
                print_title("移出黑名单");
                std::string username;
                std::cout << "请输入用户名: ";
                std::getline(std::cin, username);
                
                if (auth.unblacklist_user(current_admin, username)) {
                    std::cout << "用户 " << username << " 已移出黑名单" << std::endl;
                } else {
                    std::cout << "操作失败" << std::endl;
                }
                break;
            }
            case 7: {
                print_title("黑名单");
                std::cout << auth.get_blacklist() << std::endl;
                break;
            }
            case 8: {
                print_title("货币系统统计");
                std::cout << currency_db.export_system_statistics() << std::endl;
                break;
            }
            case 9: {
                print_title("匿名地址映射表");
                auto mappings = PrivacyUtils::get_all_mappings();
                
                if (mappings.empty()) {
                    std::cout << "暂无匿名地址映射记录" << std::endl;
                } else {
                    std::cout << "匿名地址映射关系:" << std::endl;
                    std::cout << "--------------------------------------------------" << std::endl;
                    int count = 0;
                    for (const auto& pair : mappings) {
                        count++;
                        std::cout << count << ". 真实地址: " << pair.first << std::endl;
                        std::cout << "   匿名地址: " << pair.second << std::endl;
                        std::cout << std::endl;
                    }
                    std::cout << "--------------------------------------------------" << std::endl;
                    std::cout << "共 " << count << " 条映射记录" << std::endl;
                }
                
                break;
            }
            case 10: {
                std::cout << "已退出登录" << std::endl;
                break;
            }
            default: {
                std::cout << "无效选项，请重新选择" << std::endl;
                break;
            }
        }
    }
}

// 登录界面
void login_screen(AuthManager& auth, Blockchain& blockchain, 
                  CreditManager& credit_manager, UserDatabase& user_db,
                  PendingTransactionDB& pending_tx_db,
                  CurrencyRecordDB& currency_db) {
    int choice = 0;
    
    while (choice != 4) {
        print_title("UTXO 安全电子货币交易系统 - 登录");
        std::cout << "1. 用户登录" << std::endl;
        std::cout << "2. 管理员登录" << std::endl;
        std::cout << "3. 用户注册" << std::endl;
        std::cout << "4. 退出系统" << std::endl;
        std::cout << std::endl;
        std::cout << "请选择操作: ";
        
        std::cin >> choice;
        clear_input();
        
        switch (choice) {
            case 1: {
                print_title("用户登录");
                std::string username, password;
                
                std::cout << "用户名: ";
                std::getline(std::cin, username);
                std::cout << "密码: ";
                std::getline(std::cin, password);
                
                if (auth.is_blacklisted(username)) {
                    std::cout << "错误: 您的账户已被加入黑名单" << std::endl;
                    break;
                }
                
                if (auth.login(username, password)) {
                    if (auth.is_admin(username)) {
                        std::cout << "请使用管理员登录选项" << std::endl;
                    } else {
                        std::cout << "登录成功!" << std::endl;
                        user_menu(auth, blockchain, credit_manager, user_db, pending_tx_db, currency_db, username);
                    }
                } else {
                    std::cout << "登录失败: 用户名或密码错误" << std::endl;
                }
                break;
            }
            case 2: {
                print_title("管理员登录");
                std::string username, password;
                
                std::cout << "管理员用户名: ";
                std::getline(std::cin, username);
                std::cout << "密码: ";
                std::getline(std::cin, password);
                
                if (auth.login(username, password) && auth.is_admin(username)) {
                    std::cout << "管理员登录成功!" << std::endl;
                    admin_menu(auth, blockchain, credit_manager, user_db, currency_db, username);
                } else {
                    std::cout << "登录失败: 管理员凭证无效" << std::endl;
                }
                break;
            }
            case 3: {
                print_title("用户注册");
                std::string username, password, address;
                
                std::cout << "用户名: ";
                std::getline(std::cin, username);
                std::cout << "密码: ";
                std::getline(std::cin, password);
                std::cout << "钱包地址: ";
                std::getline(std::cin, address);
                
                if (auth.register_user(username, password, address)) {
                    std::cout << "注册成功!" << std::endl;
                    
                    // 新用户自动获得20单位
                    Transaction welcome_tx;
                    welcome_tx.tx_hash = "welcome_" + username + "_" + std::to_string(time(nullptr));
                    welcome_tx.outputs.push_back({20.0, address});
                    
                    blockchain.add_special_transaction(welcome_tx);
                    
                    // RSA加密并记录到货币记录数据库
                    currency_db.record_currency_acquisition(address, 20.0, welcome_tx.tx_hash, "reward");
                    
                    std::cout << "欢迎! 已赠送20单位初始余额" << std::endl;
                    std::cout << "当前余额: " << blockchain.get_balance(address) << " 单位" << std::endl;
                } else {
                    std::cout << "注册失败: 用户名已存在" << std::endl;
                }
                break;
            }
            case 4: {
                std::cout << "感谢使用本系统，再见!" << std::endl;
                break;
            }
            default: {
                std::cout << "无效选项，请重新选择" << std::endl;
                break;
            }
        }
    }
}

int main() {
    // 设置控制台编码为UTF-8
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    print_title("UTXO 安全电子货币交易系统 - 初始化");
    
    // 创建子系统
    std::cout << "正在初始化系统..." << std::endl;
    
    Blockchain blockchain(2);
    CreditManager credit_manager;
    UserDatabase user_db("utxo_db");
    AuthManager auth;
    PendingTransactionDB pending_tx_db;
    CurrencyRecordDB currency_db;  // 新增: 货币记录RSA加密数据库
    
    std::cout << "系统初始化完成!" << std::endl;
    
    // 注册初始用户
    std::cout << "\n正在注册初始用户..." << std::endl;
    
    auth.register_user("alice", "alice123", "ALICE_ADDRESS");
    auth.register_user("bob", "bob123", "BOB_ADDRESS");
    auth.register_user("charlie", "charlie123", "CHARLIE_ADDRESS");
    
    std::cout << "初始用户注册完成!" << std::endl;
    
    // 为初始用户生成匿名地址
    std::cout << "\n正在生成匿名地址..." << std::endl;
    std::string alice_anon = PrivacyUtils::register_mapping("ALICE_ADDRESS");
    std::string bob_anon = PrivacyUtils::register_mapping("BOB_ADDRESS");
    std::string charlie_anon = PrivacyUtils::register_mapping("CHARLIE_ADDRESS");
    std::string savings_anon = PrivacyUtils::register_mapping("SAVINGS_POOL");
    
    std::cout << "匿名地址已生成!" << std::endl;
    std::cout << "  Alice: " << alice_anon << std::endl;
    std::cout << "  Bob: " << bob_anon << std::endl;
    std::cout << "  Charlie: " << charlie_anon << std::endl;
    std::cout << "  储蓄池: " << savings_anon << std::endl;
    std::cout << "\n默认管理员: admin / admin123" << std::endl;
    std::cout << "默认用户:   alice / alice123" << std::endl;
    std::cout << "            bob / bob123" << std::endl;
    std::cout << "            charlie / charlie123" << std::endl;
    
    // 创建初始资金
    std::cout << "\n正在创建初始资金..." << std::endl;
    
    Transaction savings_tx;
    savings_tx.tx_hash = "initial_savings";
    savings_tx.outputs.push_back({500.0, "ALICE_ADDRESS"});
    savings_tx.outputs.push_back({500.0, "BOB_ADDRESS"});
    savings_tx.outputs.push_back({500.0, "CHARLIE_ADDRESS"});
    savings_tx.outputs.push_back({500.0, "SAVINGS_POOL"});
    
    blockchain.add_special_transaction(savings_tx);
    blockchain.remove_genesis_savings_utxo();
    
    // RSA加密并记录初始资金
    std::cout << "\n正在RSA加密记录初始资金..." << std::endl;
    currency_db.record_currency_acquisition("ALICE_ADDRESS", 500.0, "initial_savings", "reward");
    currency_db.record_currency_acquisition("BOB_ADDRESS", 500.0, "initial_savings", "reward");
    currency_db.record_currency_acquisition("CHARLIE_ADDRESS", 500.0, "initial_savings", "reward");
    currency_db.record_currency_acquisition("SAVINGS_POOL", 500.0, "initial_savings", "reward");
    
    std::cout << "初始资金已分配并RSA加密记录!" << std::endl;
    std::cout << "  Alice:   500 单位" << std::endl;
    std::cout << "  Bob:     500 单位" << std::endl;
    std::cout << "  Charlie: 500 单位" << std::endl;
    std::cout << "  储蓄池:  500 单位" << std::endl;
    
    // 广播初始交易
    std::cout << "\n正在广播初始交易..." << std::endl;
    credit_manager.broadcast_transaction("tx_init_1", "SYSTEM", "alice", 100.0);
    credit_manager.broadcast_transaction("tx_init_2", "SYSTEM", "bob", 100.0);
    credit_manager.broadcast_transaction("tx_init_3", "SYSTEM", "charlie", 100.0);
    
    // 初始用户见证
    std::vector<std::string> init_txs = {"tx_init_1", "tx_init_2", "tx_init_3"};
    credit_manager.batch_witness_transactions("alice", init_txs);
    credit_manager.batch_witness_transactions("bob", init_txs);
    credit_manager.batch_witness_transactions("charlie", init_txs);
    
    std::cout << "初始交易广播和见证完成!" << std::endl;
    
    // 显示系统RSA密钥信息
    std::cout << "\n--- 系统RSA密钥信息 ---" << std::endl;
    std::cout << "系统密钥ID: " << currency_db.get_system_key_id() << std::endl;
    std::cout << "系统公钥: " << currency_db.get_system_public_key().substr(0, 32) << "..." << std::endl;
    
    // 进入登录界面
    login_screen(auth, blockchain, credit_manager, user_db, pending_tx_db, currency_db);
    
    return 0;
}
