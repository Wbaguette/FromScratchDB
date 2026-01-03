#pragma once
#include <cstdint>
#include <functional>
#include <utility>

#include "../shared/views.h"
#include "bnode.h"

struct BTree {
    size_t m_Root;
    struct Callbacks {
        std::function<ByteVecView(uint64_t)> get;
        std::function<uint64_t(ByteVecView)> alloc;
        std::function<void(uint64_t)> del;
    };
    Callbacks m_Callbacks;

    // Constructor
    explicit BTree(size_t root);
    explicit BTree(Callbacks cbks);
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
    void insert(ByteVecView key, ByteVecView val);
    bool remove(ByteVecView key);
};

BNode tree_insert(BTree& tree, const BNode& node, ByteVecView key, ByteVecView data);
void node_replace_kid_n(BTree& tree, BNode& new_, const BNode& old, uint16_t idx,
                        std::span<const BNode> kids);
void node_merge(BNode& new_, BNode& left, BNode& right);
void leaf_delete_key(BNode& new_, const BNode& old, uint16_t idx);
BNode tree_delete_key(BTree& tree, const BNode& node, ByteVecView key);
BNode node_delete_key(BTree& tree, const BNode& node, uint16_t idx, ByteVecView key);
void node_replace_2_kid(BNode& new_, const BNode& old, uint16_t idx, uint64_t merged,
                        ByteVecView key);
std::pair<int8_t, BNode> should_merge(BTree& tree, const BNode& node, uint16_t idx,
                                      const BNode& updated);