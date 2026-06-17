cmake_minimum_required(VERSION 3.10)
project(UTXOSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 包含头文件目录
include_directories(include)

# 添加源文件
set(SOURCES
    main.cpp
    core/utxo.cpp
    core/blockchain.cpp
    crypto/crypto.cpp
    crypto/rsa_crypto.cpp
    crypto/privacy_utils.cpp
    database/user_database.cpp
    database/user_auth.cpp
    database/pending_transaction.cpp
    database/currency_record.cpp
    system/credit_system.cpp
    system/merkle_tree.cpp
)

# 添加主程序可执行文件
add_executable(utxo_system ${SOURCES})

# 链接Windows加密库
if(WIN32)
    target_link_libraries(utxo_system bcrypt)
endif()

# 设置调试工作目录
set_target_properties(utxo_system PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
