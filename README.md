# UTXO安全电子货币交易系统

> 基于UTXO模型和多种加密技术的安全电子货币交易系统

- **仓库地址**：https://github.com/qiancheng255/UTXOSystem_r.git
- **作者**：162320220李君基
- **邮箱**：3441895472@qq.com

## 项目简介

本项目实现了一个基于UTXO（未花费交易输出）模型的安全电子货币交易系统。系统采用SHA256哈希、RSA加密、数字签名、Merkle树等多种加密技术，确保交易的安全性、匿名性和不可篡改性。

核心功能包括：
- 用户注册与认证（带盐哈希密码存储）
- 匿名地址生成（混合时间戳、随机数、线程ID等多源熵）
- UTXO模型交易（防双花攻击）
- 区块链确认（工作量证明挖矿）
- 数字签名验证
- Merkle树数据完整性验证
- 货币记录RSA加密存储

## 环境与依赖

### 运行环境

| 项目 | 版本 | 说明 |
|------|------|------|
| 操作系统 | Windows 10/11 | 使用Windows BCrypt API |
| 编译器 | Visual Studio 2019+ | 支持C++17标准 |
| C++标准 | C++17 | 使用现代C++特性 |

### 开源程序与第三方依赖

| 依赖名称 | 使用版本 | 下载链接 | 安装方式 | 说明 |
|----------|----------|----------|----------|------|
| Visual Studio | 2019+ | https://visualstudio.microsoft.com/ | 官方安装包 | C++开发环境 |
| Windows SDK | 10.0+ | 随VS安装 | 自动安装 | BCrypt API |

### 系统依赖

本项目使用Windows内置的BCrypt API进行加密操作，无需额外安装加密库。

## 配置说明

### 编译配置

1. 使用Visual Studio打开 `UTXOSystem.sln`
2. 选择 `Release` 或 `Debug` 配置
3. 选择 `x64` 平台
4. 按 `F5` 编译运行

### 命令行编译

```powershell
# 进入项目目录
cd Desktop/src

# 使用MSBuild编译
msbuild UTXOSystem.sln /p:Configuration=Release /p:Platform=x64

# 运行程序
.\x64\Release\UTXOSystem.exe
```

## 快速开始

```powershell
# 1. 克隆仓库
git clonehttps://github.com/qiancheng255/UTXOSystem_r.git
cd src

# 2. 使用Visual Studio打开解决方案
# 双击 UTXOSystem.sln

# 3. 编译运行
# 在Visual Studio中按F5，或使用命令行：
msbuild UTXOSystem.sln /p:Configuration=Release /p:Platform=x64
.\x64\Release\UTXOSystem.exe
```

## 项目结构

```
src/
├── main.cpp                    # 主程序入口
├── include/                    # 头文件目录
│   ├── crypto.h               # 加密模块接口
│   ├── rsa_crypto.h           # RSA加密接口
│   ├── utxo.h                 # UTXO模型接口
│   ├── blockchain.h           # 区块链接口
│   ├── merkle_tree.h          # Merkle树接口
│   ├── privacy_utils.h        # 匿名地址接口
│   ├── user_auth.h            # 用户认证接口
│   ├── user_database.h        # 用户数据库接口
│   ├── pending_transaction.h  # 待交易接口
│   ├── currency_record.h      # 货币记录接口
│   └── credit_system.h        # 信用系统接口
├── core/                       # 核心业务逻辑
│   ├── blockchain.cpp         # 区块链管理实现
│   └── utxo.cpp               # UTXO模型实现
├── crypto/                     # 加密模块实现
│   ├── crypto.cpp             # SHA256哈希实现
│   ├── rsa_crypto.cpp         # RSA加密实现
│   └── privacy_utils.cpp      # 匿名地址实现
├── database/                   # 数据存储层
│   ├── user_auth.cpp          # 用户认证实现
│   ├── user_database.cpp      # 用户数据库实现
│   ├── pending_transaction.cpp# 待交易数据库实现
│   └── currency_record.cpp    # 货币记录实现
├── system/                     # 系统功能模块
│   ├── merkle_tree.cpp        # Merkle树实现
│   └── credit_system.cpp      # 信用系统实现
├── UTXOSystem.sln              # VS解决方案文件
├── UTXOSystem.vcxproj          # VS项目文件
├── CMakeLists.txt              # CMake构建配置
└── .gitignore                  # Git忽略配置
```

## 核心模块说明

### 1. 加密模块 (crypto/)
- SHA256哈希算法实现
- 使用Windows BCrypt API
- 提供数据完整性验证
- RSA密钥对生成与加密
- 数字签名与验证

### 2. 核心业务 (core/)
- UTXO模型实现
- 区块链数据结构
- 工作量证明挖矿
- 交易创建与验证

### 3. 数据存储 (database/)
- 用户认证与数据库
- 待交易队列管理
- 货币记录加密存储

### 4. 系统功能 (system/)
- Merkle树构建与验证
- 信用系统管理

### 5. 匿名地址 (crypto/privacy_utils.cpp)
- 多源熵混合生成
- 交易地址随机化
- 隐私保护机制

## 安全特性

- **SHA256哈希**：数据完整性、密码存储
- **RSA加密**：敏感数据机密性
- **数字签名**：身份认证、不可否认性
- **Merkle树**：交易完整性验证
- **UTXO模型**：防双花攻击
- **匿名地址**：用户隐私保护
- **互斥锁**：并发安全
- **带盐哈希**：密码安全存储
- **工作量证明**：防篡改、公平发行

## 许可证

本项目仅供学习研究使用。
