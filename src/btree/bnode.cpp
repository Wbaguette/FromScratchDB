#include "bnode.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "../shared/treesizes.h"
#include "../utils/bytes.h"

BNode::BNode(size_t size) : m_Data(size) {}

BNode::BNode(ByteVecView data) : m_Data(data.begin(), data.end()) {}

std::ostream& operator<<(std::ostream& os, const BNode& b_node) {
    os << "BNode { " << "\n";
    std::string_view bnode_type;
    if (b_node.btype() == 1) {
        bnode_type = "node(1)";
    } else {
        bnode_type = "leaf(2)";
    }

    os << " btype=" << bnode_type << "\n"
       << " nkeys=" << b_node.nkeys() << "\n"
       << " m_Data_vec_size=" << b_node.m_Data.size() << "\n"
       << " m_Data_vec_capacity=" << b_node.m_Data.capacity() << "\n";

    os << " m_Data=[ ";
    for (uint8_t c : b_node.m_Data) {
        os << static_cast<char>(c);
    }
    os << "]" << "\n";
    os << "}" << "\n";

    return os;
}

uint16_t BNode::btype() const {
    if (m_Data.size() < 2) {
        throw std::out_of_range("BNode m_Data too small to have btype");
    }
    return read_le16(0);
}

uint16_t BNode::nkeys() const {
    if (m_Data.size() < 4) {
        throw std::out_of_range("BNode m_Data too small to have nkeys");
    }
    return read_le16(2);
}

uint16_t BNode::nbytes() const { return kv_pos(nkeys()); }

uint64_t BNode::get_ptr(uint16_t idx) const {
    if (idx >= nkeys()) {
        throw std::out_of_range("child pointer idx is greater than number of keys");
    }

    // Position of child pointer is 4 bytes (header) + 8 * position (each child pointer is 8 bytes
    // on 64bit arch)
    size_t pos = 4 + (8 * static_cast<size_t>(idx));
    return read_le64(pos);
}

void BNode::set_ptr(uint16_t idx, uint64_t val) {
    if (idx >= nkeys()) {
        throw std::out_of_range("child pointer idx is greater than number of keys");
    }

    size_t pos =
        4 +
        (8 * static_cast<size_t>(idx));  // Position of child pointer is 4 bytes (header) + 8 *
                                         // position (each child pointer is 8 bytes on 64bit arch)
    write_le64(pos, val);
}

uint16_t BNode::get_offset(uint16_t idx) const {
    if (idx == 0) {
        return 0;
    }

    size_t pos = 4 + (8 * static_cast<size_t>(nkeys())) + (2 * (static_cast<size_t>(idx) - 1));
    return read_le16(pos);
}

void BNode::set_offset(uint16_t idx, uint16_t offset) {
    if (idx > nkeys()) {
        throw std::out_of_range("offset pos idx is greater than number of keys");
    }
    if (idx == 0) {
        throw std::out_of_range("offset pos idx cannot be 0");
    }

    size_t pos = 4 + (8 * static_cast<size_t>(nkeys())) + (2 * (static_cast<size_t>(idx) - 1));
    write_le16(pos, offset);
}

uint16_t BNode::kv_pos(uint16_t idx) const {
    if (idx > nkeys()) {
        throw std::out_of_range("kv idx is greater than number of keys");
    }

    return 4 + (8 * nkeys()) + (2 * nkeys()) + get_offset(idx);
}

ByteVecView BNode::get_key(uint16_t idx) const {
    if (idx >= nkeys()) {
        throw std::out_of_range("key index is greater than number of keys");
    }

    auto pos = static_cast<size_t>(kv_pos(idx));
    uint16_t key_length = read_le16(pos);

    return {&m_Data[pos + 4], key_length};
}

ByteVecView BNode::get_val(uint16_t idx) const {
    if (idx >= nkeys()) {
        throw std::out_of_range("val index is greater than number of keys");
    }

    size_t pos = kv_pos(idx);
    uint16_t key_length = read_le16(pos);
    uint16_t val_length = read_le16(pos + 2);

    return {&m_Data[pos + 4 + key_length], val_length};
}

void leaf_insert(BNode& new_, const BNode& old, uint16_t idx, ByteVecView key, ByteVecView val) {
    new_.set_header(BNODE_LEAF, old.nkeys() + 1);
    node_append_range(new_, old, 0, 0, idx);
    node_append_kv(new_, idx, 0, key, val);
    node_append_range(new_, old, idx + 1, idx, old.nkeys() - idx);
}

void leaf_update(BNode& new_, const BNode& old, uint16_t idx, ByteVecView key, ByteVecView val) {
    new_.set_header(BNODE_LEAF, old.nkeys());
    node_append_range(new_, old, 0, 0, idx);
    node_append_kv(new_, idx, 0, key, val);
    node_append_range(new_, old, idx + 1, idx + 1, old.nkeys() - (idx + 1));
}

void node_append_range(BNode& new_, const BNode& old, uint16_t dst_new, uint16_t src_old,
                       uint16_t n) {
    for (size_t i = 0; i < n; i++) {
        uint16_t dst = dst_new + static_cast<uint16_t>(i);
        uint16_t src = src_old + static_cast<uint16_t>(i);
        node_append_kv(new_, dst, old.get_ptr(src), old.get_key(src), old.get_val(src));
    }
}

void node_append_kv(BNode& new_, uint16_t idx, uint64_t ptr, ByteVecView key, ByteVecView val) {
    new_.set_ptr(idx, ptr);
    size_t pos = new_.kv_pos(idx);

    new_.write_le16(pos, static_cast<uint16_t>(key.size()));
    new_.write_le16(pos + 2, static_cast<uint16_t>(val.size()));

    std::memcpy(&new_.m_Data[pos + 4], key.data(), key.size());
    std::memcpy(&new_.m_Data[pos + 4 + key.size()], val.data(), val.size());

    new_.set_offset(idx + 1,
                    new_.get_offset(idx) + 4 + static_cast<uint16_t>(key.size() + val.size()));
}

// Find the last position that is less than or equal to the key
uint16_t lookup_le_pos(const BNode& node, ByteVecView key) {
    uint16_t n_keys = node.nkeys();
    size_t i = 0;
    // TODO: Could possible be binary search
    for (; i < n_keys; i++) {
        const ByteVecView this_key = node.get_key(static_cast<uint16_t>(i));
        int cmp = lex_cmp_byte_vecs(this_key, key);
        // 0 if a == b, -1 if a < b, and +1 if a > b.
        if (cmp == 0) {
            return static_cast<uint16_t>(i);
        }  // Equal means this index is where they are <=
        if (cmp > 0) {
            return static_cast<uint16_t>(i) - 1;
        }  // a > b therefore the previous index is where it is <=
    }

    return static_cast<uint16_t>(i) - 1;
}

void split_half(BNode& left, BNode& right, const BNode& old) {
    if (old.nkeys() < 2) {
        throw std::out_of_range("Too little keys to split into two");
    }
    // The idea is to find how many bytes to put into the left BNode while being within the page
    // size
    size_t n_left = old.nkeys() / 2;
    while (true) {
        size_t left_bytes =
            4 + (8 * n_left) + (2 * n_left) + old.get_offset(static_cast<uint16_t>(n_left));
        if (n_left == 0 || left_bytes <= static_cast<size_t>(BTREE_PAGE_SIZE)) {
            break;
        }
        n_left--;
    }
    if (n_left < 1) {
        throw std::length_error("Cannot split when left BNode will be size 0");
    }
    // Try to equalize the number of bytes for the right BNode
    while (true) {
        size_t left_bytes =
            4 + (8 * n_left) + (2 * n_left) + old.get_offset(static_cast<uint16_t>(n_left));
        size_t right_bytes = old.nbytes() - left_bytes + 4;
        if (right_bytes <= static_cast<size_t>(BTREE_PAGE_SIZE)) {
            break;
        }
        n_left++;
    }
    if (n_left >= old.nkeys()) {
        throw std::length_error("Cannot split when right BNode will be size 0");
    }

    size_t n_right = static_cast<size_t>(old.nkeys()) - n_left;

    left.m_Data.clear();
    left.set_header(old.btype(), static_cast<uint16_t>(n_left));
    node_append_range(left, old, 0, 0, static_cast<uint16_t>(n_left));

    right.m_Data.clear();
    right.set_header(old.btype(), static_cast<uint16_t>(n_right));
    node_append_range(right, old, 0, static_cast<uint16_t>(n_left), static_cast<uint16_t>(n_right));

    if (right.nbytes() > static_cast<uint16_t>(BTREE_PAGE_SIZE)) {
        throw std::length_error("Right BNode bytes size exceed max BTREE_PAGE_SIZE");
    }
}

std::span<const BNode> try_split_thrice(BNode& old) {
    if (old.nbytes() <= static_cast<uint16_t>(BTREE_PAGE_SIZE)) {
        old.m_Data.resize(BTREE_PAGE_SIZE);
        return std::vector<BNode>{old};
    }

    BNode left(2 * BTREE_PAGE_SIZE);
    BNode right;
    split_half(left, right, old);
    if (left.nbytes() <= static_cast<uint16_t>(BTREE_PAGE_SIZE)) {
        left.m_Data.resize(BTREE_PAGE_SIZE);
        return std::vector<BNode>{left, right};
    }

    // If we get here that means left is too big and we need to split again
    BNode leftleft;
    BNode middle;
    split_half(leftleft, middle, left);

    if (leftleft.nbytes() > static_cast<uint16_t>(BTREE_PAGE_SIZE)) {
        throw std::length_error("Leftleft bytes size exceed max BTREE_PAGE_SIZE");
    }

    return std::vector<BNode>{leftleft, middle, left};
}
