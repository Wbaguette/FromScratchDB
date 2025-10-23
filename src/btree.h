#pragma once
#include <cstdint>
#include <memory>
#include <vector>

constexpr int BNODE_NODE = 1;
constexpr int BNODE_LEAF = 2; 

constexpr int BTREE_PAGE_SIZE = 4096;
constexpr int BTREE_MAX_KEY_SIZE = 1000;
constexpr int BTREE_MAX_VAL_SIZE = 3000;

class Node {
public:
    std::vector<std::vector<uint8_t>> keys;
    std::vector<std::vector<uint8_t>> vals;
    std::vector<std::unique_ptr<Node>> kids;

    ~Node() = default;
    Node();
    std::vector<uint8_t> encode();
    static std::unique_ptr<Node> decode(std::vector<uint8_t> page);

private:
    
};

class BNode {
public:
    std::vector<uint8_t> data;

    BNode(size_t size = 0);
    uint16_t btype() const;
    uint16_t nkeys() const;
    void set_header(uint16_t btype, uint16_t nkeys);
    uint64_t get_ptr(uint16_t idx) const;
    void set_ptr(uint16_t idx, uint64_t val);
    uint16_t get_offset(uint16_t idx);
    uint16_t kv_pos(uint16_t idx);
    std::vector<uint8_t> get_key(uint16_t idx);
    std::vector<uint8_t> get_val(uint16_t idx);


private:
    uint16_t read_le16(size_t offset) const;
    void write_le16(size_t offset, uint16_t val);
};

