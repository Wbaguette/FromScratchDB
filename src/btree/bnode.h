#pragma once
#include <cstdint>
#include <ostream>
#include <vector>

#include "../shared/treesizes.h"
#include "../shared/views.h"

constexpr int BNODE_NODE = 1;
constexpr int BNODE_LEAF = 2;

struct BNode {
    std::vector<uint8_t> m_Data;

    // Constructor
    explicit BNode(size_t size = BTREE_PAGE_SIZE);
    BNode(ByteVecView data);
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
    [[nodiscard]] uint16_t btype() const;
    [[nodiscard]] uint16_t nkeys() const;
    [[nodiscard]] uint16_t nbytes() const;
    void set_header(uint16_t btype, uint16_t nkeys) {
        write_le16(0, btype);
        write_le16(2, nkeys);
    }
    [[nodiscard]] uint64_t get_ptr(uint16_t idx) const;
    void set_ptr(uint16_t idx, uint64_t val);
    [[nodiscard]] uint16_t get_offset(uint16_t idx) const;
    void set_offset(uint16_t idx, uint16_t offset);
    [[nodiscard]] uint16_t kv_pos(uint16_t idx) const;
    [[nodiscard]] ByteVecView get_key(uint16_t idx) const;
    [[nodiscard]] ByteVecView get_val(uint16_t idx) const;

    friend std::ostream& operator<<(std::ostream& os, const BNode& b_node);

    [[nodiscard]] uint16_t read_le16(size_t offset) const {
        uint16_t v;
        std::memcpy(&v, &m_Data[offset], sizeof(v));
        return v;
    }

    [[nodiscard]] uint64_t read_le64(size_t offset) const {
        uint64_t v;
        std::memcpy(&v, &m_Data[offset], sizeof(v));
        return v;
    }

    void write_le16(size_t offset, uint16_t val) {
        if (m_Data.size() < offset + sizeof(val)) {
            m_Data.resize(offset + sizeof(val));
        }
        std::memcpy(&m_Data[offset], &val, sizeof(val));
    }

    void write_le64(size_t offset, uint64_t val) {
        if (m_Data.size() < offset + sizeof(val)) {
            m_Data.resize(offset + sizeof(val));
        }
        std::memcpy(&m_Data[offset], &val, sizeof(val));
    }
};

void node_append_kv(BNode& new_, uint16_t idx, uint64_t ptr, ByteVecView key, ByteVecView val);
std::span<const BNode> try_split_thrice(BNode& old);
void split_half(BNode& left, BNode& right, const BNode& old);
uint16_t lookup_le_pos(const BNode& node, ByteVecView key);
void node_append_range(BNode& new_, const BNode& old, uint16_t dst_new, uint16_t src_old,
                       uint16_t n);
void leaf_update(BNode& new_, const BNode& old, uint16_t idx, ByteVecView key, ByteVecView val);
void leaf_insert(BNode& new_, const BNode& old, uint16_t idx, ByteVecView key, ByteVecView val);
