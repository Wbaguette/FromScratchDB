#pragma once
#include <cstdint>
#include <memory>
#include <vector>

constexpr int BTREE_PAGE_SIZE = 4096;
constexpr int BTREE_MAX_KEY_SIZE = 1000;
constexpr int BTREE_MAX_VAL_SIZE = 3000;

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

    ~BTree() = default;
    BTree(size_t root);

};