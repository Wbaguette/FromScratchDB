#pragma once
#include "btree.h"
#include <cstdint>
#include <ostream>
#include <vector>
#include <span>

constexpr int BNODE_NODE = 1;
constexpr int BNODE_LEAF = 2; 

// Non-owning, readonly, vector of bytes
typedef std::span<const uint8_t> ByteVecView;

struct BNode {
public:
    std::vector<uint8_t> m_Data;

    // Constructor
    BNode(size_t size = BTREE_PAGE_SIZE);
    // Copy constructor
    BNode(const BNode& other) = default;
    // Destructor
    ~BNode() = default;
    // Copy assignment constructor
    BNode& operator=(const BNode& other) = default;
    // Move constructor
    BNode(BNode&& other) noexcept = default;
    // Move assignment 
    BNode& operator=(BNode&& other) noexcept = default;

    uint16_t btype() const;
    uint16_t nkeys() const;
    uint16_t nbytes() const;
    inline void set_header(uint16_t btype, uint16_t nkeys) {
        write_le16(0, btype);
        write_le16(2, nkeys);
    }
    uint64_t get_ptr(uint16_t idx) const;
    void set_ptr(uint16_t idx, uint64_t val);
    uint16_t get_offset(uint16_t idx) const;
    void set_offset(uint16_t idx, uint16_t offset);
    uint16_t kv_pos(uint16_t idx) const ;
    ByteVecView get_key(uint16_t idx) const;
    ByteVecView get_val(uint16_t idx) const;
    void append_kv(uint16_t idx, uint64_t ptr, ByteVecView key, ByteVecView val);
    void leaf_insert(const BNode& old, uint16_t idx, ByteVecView key, ByteVecView val);
    void leaf_update(const BNode& old, uint16_t idx, ByteVecView key, ByteVecView val);
    void append_range(const BNode& old, uint16_t dst_new, uint16_t src_old, uint16_t n);
    uint16_t lookup_le_pos(ByteVecView key) const;
    void split_half(BNode& left, BNode& right) const;
    std::vector<BNode> try_split_thrice();

    friend std::ostream& operator<<(std::ostream& os, const BNode& b_node);

private:
    inline uint16_t read_le16(size_t offset) const {
        uint16_t v;
        std::memcpy(&v, &m_Data[offset], sizeof(v));
        return v;
    }

    inline uint64_t read_le64(size_t offset) const {
        uint64_t v;
        std::memcpy(&v, &m_Data[offset], sizeof(v));
        return v;
    }

    inline void write_le16(size_t offset, uint16_t val) {
        if (m_Data.size() < offset + sizeof(val))
            m_Data.resize(offset + sizeof(val));
        std::memcpy(&m_Data[offset], &val, sizeof(val));
    }

    inline void write_le64(size_t offset, uint64_t val) {
        if (m_Data.size() < offset + sizeof(val))
            m_Data.resize(offset + sizeof(val));
        std::memcpy(&m_Data[offset], &val, sizeof(val));
    }
};
