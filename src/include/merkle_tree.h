/**
 * Merkle树模块头文件
 * 实现交易完整性验证的Merkle树结构
 */

#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <string>
#include <vector>
#include <memory>

/**
 * Merkle树节点
 */
struct MerkleNode {
    std::string hash;                           // 节点哈希值
    std::shared_ptr<MerkleNode> left;           // 左子节点
    std::shared_ptr<MerkleNode> right;          // 右子节点
    std::string data;                           // 叶子节点的原始数据（仅叶子节点有）
    
    MerkleNode();
    MerkleNode(const std::string& h);
    MerkleNode(const std::string& h, const std::string& d);
};

/**
 * Merkle证明路径节点
 */
struct MerkleProofNode {
    std::string hash;       // 兄弟节点的哈希
    bool is_left;           // 兄弟节点是否在左边
    
    MerkleProofNode();
    MerkleProofNode(const std::string& h, bool left);
};

/**
 * Merkle树
 * 用于验证交易数据的完整性
 */
class MerkleTree {
private:
    std::shared_ptr<MerkleNode> root;           // 根节点
    std::vector<std::string> leaves;            // 叶子节点哈希列表
    std::vector<std::string> data_list;         // 原始数据列表
    
    /**
     * 计算SHA256哈希
     */
    std::string sha256(const std::string& input) const;
    
    /**
     * 递归构建Merkle树
     * @param nodes 当前层级的节点列表
     * @return 根节点
     */
    std::shared_ptr<MerkleNode> build_tree(
        std::vector<std::shared_ptr<MerkleNode>> nodes);
    
    /**
     * 递归收集证明路径
     */
    bool collect_proof(const std::shared_ptr<MerkleNode>& node,
                      const std::string& target_hash,
                      std::vector<MerkleProofNode>& proof,
                      int depth) const;
    
public:
    MerkleTree();
    
    /**
     * 构建Merkle树
     * @param data_list 原始数据列表（交易哈希）
     */
    void build(const std::vector<std::string>& data_list);
    
    /**
     * 获取Merkle根哈希
     * @return 根哈希值
     */
    std::string get_root_hash() const;
    
    /**
     * 获取叶子节点数量
     * @return 叶子节点数
     */
    size_t get_leaf_count() const;
    
    /**
     * 生成Merkle证明
     * @param index 叶子节点索引
     * @return 证明路径
     */
    std::vector<MerkleProofNode> generate_proof(size_t index) const;
    
    /**
     * 验证Merkle证明
     * @param data 原始数据
     * @param proof 证明路径
     * @param expected_root 预期的根哈希
     * @return 验证是否通过
     */
    static bool verify_proof(const std::string& data,
                            const std::vector<MerkleProofNode>& proof,
                            const std::string& expected_root);
    
    /**
     * 验证指定索引的数据是否有效
     * @param index 数据索引
     * @return 验证是否通过
     */
    bool verify(size_t index) const;
    
    /**
     * 打印Merkle树结构（调试用）
     */
    void print_tree() const;
    
    /**
     * 获取所有叶子节点哈希
     * @return 叶子节点哈希列表
     */
    std::vector<std::string> get_leaves() const;
};

#endif // MERKLE_TREE_H
