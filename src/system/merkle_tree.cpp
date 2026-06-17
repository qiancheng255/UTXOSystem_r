/**
 * Merkle树模块实现文件
 * 实现交易完整性验证的Merkle树结构
 */

#include "merkle_tree.h" 
#include "crypto.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <string>
#include <queue>

// MerkleNode实现
MerkleNode::MerkleNode() {}

MerkleNode::MerkleNode(const std::string& h) : hash(h) {}

MerkleNode::MerkleNode(const std::string& h, const std::string& d) 
    : hash(h), data(d) {}

// MerkleProofNode实现
MerkleProofNode::MerkleProofNode() : is_left(false) {}

MerkleProofNode::MerkleProofNode(const std::string& h, bool left) 
    : hash(h), is_left(left) {}

// MerkleTree实现
MerkleTree::MerkleTree() {}

std::string MerkleTree::sha256(const std::string& input) const {
    return Crypto::sha256(input);
}

void MerkleTree::build(const std::vector<std::string>& data) {
    data_list = data;
    leaves.clear();
    
    if (data.empty()) {
        root = nullptr;
        return;
    }
    
    // 步骤1: 计算每个数据的哈希作为叶子节点
    std::vector<std::shared_ptr<MerkleNode>> leaf_nodes;
    for (const auto& d : data) {
        std::string hash = sha256(d);
        leaves.push_back(hash);
        auto node = std::make_shared<MerkleNode>(hash, d);
        leaf_nodes.push_back(node);
    }
    
    // 步骤2: 如果叶子节点数量为奇数，复制最后一个
    if (leaf_nodes.size() % 2 != 0) {
        leaf_nodes.push_back(leaf_nodes.back());
    }
    
    // 步骤3: 递归构建树
    root = build_tree(leaf_nodes);
}

std::shared_ptr<MerkleNode> MerkleTree::build_tree(
    std::vector<std::shared_ptr<MerkleNode>> nodes) {
    
    // 如果只有一个节点，它就是根节点
    if (nodes.size() == 1) {
        return nodes[0];
    }
    
    // 如果节点数量为奇数，复制最后一个
    if (nodes.size() % 2 != 0) {
        nodes.push_back(nodes.back());
    }
    
    // 构建父节点
    std::vector<std::shared_ptr<MerkleNode>> parents;
    for (size_t i = 0; i < nodes.size(); i += 2) {
        // 父节点哈希 = SHA256(左子节点哈希 + 右子节点哈希)
        std::string combined = nodes[i]->hash + nodes[i + 1]->hash;
        std::string parent_hash = sha256(combined);
        
        auto parent = std::make_shared<MerkleNode>(parent_hash);
        parent->left = nodes[i];
        parent->right = nodes[i + 1];
        parents.push_back(parent);
    }
    
    // 递归构建上层
    return build_tree(parents);
}

std::string MerkleTree::get_root_hash() const {
    if (root) {
        return root->hash;
    }
    return "";
}

size_t MerkleTree::get_leaf_count() const {
    return leaves.size();
}

std::vector<MerkleProofNode> MerkleTree::generate_proof(size_t index) const {
    std::vector<MerkleProofNode> proof;
    
    if (!root || index >= leaves.size()) {
        return proof;
    }
    
    // 从根节点开始，收集证明路径
    collect_proof(root, leaves[index], proof, 0);
    
    return proof;
}

bool MerkleTree::collect_proof(const std::shared_ptr<MerkleNode>& node,
                              const std::string& target_hash,
                              std::vector<MerkleProofNode>& proof,
                              int depth) const {
    if (!node) {
        return false;
    }
    
    // 如果是叶子节点
    if (!node->left && !node->right) {
        return node->hash == target_hash;
    }
    
    // 检查左子树
    if (collect_proof(node->left, target_hash, proof, depth + 1)) {
        // 目标在左子树，添加右兄弟节点
        if (node->right) {
            proof.push_back(MerkleProofNode(node->right->hash, false));
        }
        return true;
    }
    
    // 检查右子树
    if (collect_proof(node->right, target_hash, proof, depth + 1)) {
        // 目标在右子树，添加左兄弟节点
        if (node->left) {
            proof.push_back(MerkleProofNode(node->left->hash, true));
        }
        return true;
    }
    
    return false;
}

bool MerkleTree::verify_proof(const std::string& data,
                             const std::vector<MerkleProofNode>& proof,
                             const std::string& expected_root) {
    // 计算数据的初始哈希
    std::string current_hash = Crypto::sha256(data);
    
    // 沿着证明路径向上计算
    for (const auto& proof_node : proof) {
        std::string combined;
        if (proof_node.is_left) {
            // 兄弟节点在左边
            combined = proof_node.hash + current_hash;
        } else {
            // 兄弟节点在右边
            combined = current_hash + proof_node.hash;
        }
        current_hash = Crypto::sha256(combined);
    }
    
    // 验证计算结果是否等于预期的根哈希
    return current_hash == expected_root;
}

bool MerkleTree::verify(size_t index) const {
    if (index >= data_list.size() || index >= leaves.size()) {
        return false;
    }
    
    // 生成证明
    auto proof = generate_proof(index);
    
    // 验证证明
    return verify_proof(data_list[index], proof, get_root_hash());
}

void MerkleTree::print_tree() const {
    if (!root) {
        std::cout << "空树" << std::endl;
        return;
    }
    
    std::cout << "\n=== Merkle树结构 ===" << std::endl;
    std::cout << "根哈希: " << root->hash.substr(0, 32) << "..." << std::endl;
    std::cout << "叶子节点数: " << leaves.size() << std::endl;
    
    // 使用BFS打印树结构
    std::queue<std::pair<std::shared_ptr<MerkleNode>, int>> q;
    q.push({root, 0});
    int current_level = -1;
    
    while (!q.empty()) {
        auto [node, level] = q.front();
        q.pop();
        
        if (level != current_level) {
            current_level = level;
            std::cout << "\n层级 " << level << ": ";
        }
        
        if (node) {
            std::cout << "[" << node->hash.substr(0, 8) << "...] ";
            
            if (node->left) q.push({node->left, level + 1});
            if (node->right) q.push({node->right, level + 1});
        }
    }
    
    std::cout << "\n" << std::endl;
    
    // 打印叶子节点
    std::cout << "叶子节点详情:" << std::endl;
    for (size_t i = 0; i < leaves.size(); i++) {
        std::cout << "  [" << i << "] " << leaves[i].substr(0, 32) << "..." << std::endl;
    }
}

std::vector<std::string> MerkleTree::get_leaves() const {
    return leaves;
}
