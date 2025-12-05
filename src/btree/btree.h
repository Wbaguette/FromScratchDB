#pragma once
#include <cstdint>
#include <memory>
#include <ostream>
#include <vector>

constexpr int BNODE_NODE = 1;
constexpr int BNODE_LEAF = 2; 

constexpr int BTREE_PAGE_SIZE = 4096;
constexpr int BTREE_MAX_KEY_SIZE = 1000;
constexpr int BTREE_MAX_VAL_SIZE = 3000;

struct Node {
public:
    std::vector<std::vector<uint8_t>> keys;
    std::vector<std::vector<uint8_t>> vals;
    std::vector<std::unique_ptr<Node>> kids;

    ~Node() = default;
    Node();
    std::vector<uint8_t> encode();
    static std::unique_ptr<Node> decode(std::vector<uint8_t> page);

};

struct BNode {
public:
    std::vector<uint8_t> data;

    BNode(size_t size = BTREE_PAGE_SIZE);
    uint16_t btype() const;
    uint16_t nkeys() const;
    uint16_t nbytes();
    inline void set_header(uint16_t btype, uint16_t nkeys) {
        write_le16(0, btype);
        write_le16(2, nkeys);
    }
    uint64_t get_ptr(uint16_t idx) const;
    void set_ptr(uint16_t idx, uint64_t val);
    uint16_t get_offset(uint16_t idx) const;
    void set_offset(uint16_t idx, uint16_t offset);
    uint16_t kv_pos(uint16_t idx) const ;
    std::vector<uint8_t> get_key(uint16_t idx) const;
    std::vector<uint8_t> get_val(uint16_t idx) const;
    void append_kv(uint16_t idx, uint64_t ptr, const std::vector<uint8_t>& key, const std::vector<uint8_t>& val);
    void leaf_insert(const BNode& old, uint16_t idx, const std::vector<uint8_t>& key, const std::vector<uint8_t>& val);
    void append_range(const BNode& old, uint16_t dst_new, uint16_t src_old, uint16_t n);
    friend std::ostream& operator<<(std::ostream& os, const BNode& b_node);

private:
    inline uint16_t read_le16(size_t offset) const {
        uint16_t v;
        std::memcpy(&v, &data[offset], sizeof(v));
        return v;
    }

    inline uint64_t read_le64(size_t offset) const {
        uint64_t v;
        std::memcpy(&v, &data[offset], sizeof(v));
        return v;
    }

    inline void write_le16(size_t offset, uint16_t val) {
        if (data.size() < offset + sizeof(val))
            data.resize(offset + sizeof(val));
        std::memcpy(&data[offset], &val, sizeof(val));
    }

    inline void write_le64(size_t offset, uint64_t val) {
        if (data.size() < offset + sizeof(val))
            data.resize(offset + sizeof(val));
        std::memcpy(&data[offset], &val, sizeof(val));
    }

};

