#pragma once
#include "../shared/bytevecview.h"
#include "bnode.h"
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

struct Node {
public:
    std::vector<std::vector<uint8_t>> m_Keys;
    std::vector<std::vector<uint8_t>> m_Vals;
    std::vector<std::unique_ptr<Node>> m_Kids;

    ~Node() = default;
    Node();
    std::vector<uint8_t> encode();
    static std::unique_ptr<Node> decode(std::vector<uint8_t> page);
};

struct BTree {
public:
    size_t m_Root;

    // Constructor
    explicit BTree(size_t root);
    // Copy constructor
    BTree(const BTree& other) = default;
    // Destructor
    ~BTree() = default;
    // Copy assignment constructor
    BTree& operator=(const BTree& other) = default;
    // Move constructor
    BTree(BTree&& other) noexcept = default;
    // Move assignment 
    BTree& operator=(BTree&& other) noexcept = default;
    ByteVecView get(uint64_t page_num) const;
    uint64_t alloc(ByteVecView data);
    void del(uint64_t page_num);
    void insert(ByteVecView key, ByteVecView val);
    bool remove(ByteVecView key);


};

BNode tree_insert(BTree& tree, const BNode& node, ByteVecView key, ByteVecView data);
void node_replace_kid_n(BTree& tree, BNode& new_, const BNode& old, uint16_t idx, std::span<const BNode> kids);
void node_merge(BNode& new_, BNode& left, BNode& right);
void leaf_delete_key(BNode& new_, const BNode& old, uint16_t idx);
BNode tree_delete_key(BTree& tree, const BNode& node, ByteVecView key);
BNode node_delete_key(BTree& tree, const BNode& node, uint16_t idx, ByteVecView key);
void node_replace_2_kid(BNode& new_, const BNode& old, uint16_t idx, uint64_t merged, ByteVecView key);
std::pair<int8_t, BNode> should_merge(BTree& tree, const BNode& node, uint16_t idx, const BNode& updated);