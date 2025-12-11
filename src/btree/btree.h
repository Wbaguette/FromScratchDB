#pragma once
#include "../shared/bytevecview.h"
#include "bnode.h"
#include <cstdint>
#include <memory>
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
    BTree(size_t root);
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
    ByteVecView get(uint64_t);
    uint64_t alloc(ByteVecView);
    void del(uint64_t);
    BNode insert_node(const BNode& node, ByteVecView key, ByteVecView data);

};