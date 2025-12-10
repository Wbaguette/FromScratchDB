#include "btree.h"
#include "../utils/bytes.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>
#include <iostream>

Node::Node() {
    static_assert(4 + 8 + 2 + 4 + BTREE_MAX_KEY_SIZE + BTREE_MAX_VAL_SIZE <= BTREE_PAGE_SIZE, "Node size is greater than max btree page size");
}

std::vector<uint8_t> Node::encode() {}

std::unique_ptr<Node> Node::decode(std::vector<uint8_t> page) {}





BNode::BNode(size_t size): data(size) {} 

std::ostream& operator<<(std::ostream& os, const BNode& b_node) {
    os << "BNode { " << std::endl; 
    std::string_view bnode_type;
    if (b_node.btype() == 1) {
        bnode_type = "node(1)";
    } else {
        bnode_type = "leaf(2)";
    }

    os << " btype=" << bnode_type << std::endl
       << " nkeys=" << b_node.nkeys() << std::endl
       << " data_vec_size=" << b_node.data.size() << std::endl
       << " data_vec_capacity=" << b_node.data.capacity() << std::endl;
    
    os << " data=[ ";
    for (uint8_t c : b_node.data) {
        os << static_cast<char>(c);
    }
    os << "]" << std::endl;
    os << "}" << std::endl;
    
    return os;
}


uint16_t BNode::btype() const {
    if (data.size() < 2)
        throw std::out_of_range("BNode data too small to have btype");
    return read_le16(0);
}

uint16_t BNode::nkeys() const {
    if (data.size() < 4)
        throw std::out_of_range("BNode data too small to have nkeys");
    return read_le16(2);    
}

uint16_t BNode::nbytes() const {
    return kv_pos(nkeys());    
}

uint64_t BNode::get_ptr(uint16_t idx) const {
    if (idx >= nkeys()) 
        throw std::out_of_range("child pointer idx is greater than number of keys");

    size_t pos = 4 + 8 * idx; // Position of child pointer is 4 bytes (header) + 8 * position (each child pointer is 8 bytes on 64bit arch)
    return read_le64(pos);
}

void BNode::set_ptr(uint16_t idx, uint64_t val) {
    if (idx >= nkeys()) 
        throw std::out_of_range("child pointer idx is greater than number of keys");

    size_t pos = 4 + 8 * idx; // Position of child pointer is 4 bytes (header) + 8 * position (each child pointer is 8 bytes on 64bit arch)
    write_le64(pos, val);
}

uint16_t BNode::get_offset(uint16_t idx) const {
    if (idx == 0) 
        return 0;   

    size_t pos = 4 + 8 * nkeys() + 2 * (idx - 1);
    return read_le16(pos);
}

void BNode::set_offset(uint16_t idx, uint16_t offset) {
    if (idx > nkeys())
        throw std::out_of_range("offset pos idx is greater than number of keys");

    size_t pos = 4 + 8 * nkeys() + 2 * (idx - 1);
    write_le16(pos, offset);
}   


uint16_t BNode::kv_pos(uint16_t idx) const {
    if (idx > nkeys()) 
        throw std::out_of_range("kv idx is greater than number of keys");

    return 4 + 8 * nkeys() + 2 * nkeys() + get_offset(idx);
}

ByteVecView BNode::get_key(uint16_t idx) const {
    if (idx >= nkeys()) 
        throw std::out_of_range("key index is greater than number of keys");

    size_t pos = kv_pos(idx);
    uint16_t key_length = read_le16(pos);


    // auto begin = data.begin() + pos + 4;
    // auto last = begin + key_length;
    // return std::vector<uint8_t>(begin, last);
    return ByteVecView(&data[pos + 4], key_length);
}

ByteVecView BNode::get_val(uint16_t idx) const {
    if (idx >= nkeys()) 
        throw std::out_of_range("val index is greater than number of keys");

    size_t pos = kv_pos(idx);
    uint16_t key_length = read_le16(pos);
    uint16_t val_length = read_le16(pos + 2);

    // auto begin = data.begin() + pos + 4 + key_length;
    // auto last   = begin + val_length;

    // return std::vector<uint8_t>(begin, last);
    return ByteVecView(&data[pos + 4 + key_length], val_length);
}

void BNode::append_kv(uint16_t idx, uint64_t ptr, ByteVecView key, ByteVecView val) {
    set_ptr(idx, ptr);
    size_t pos = kv_pos(idx);
    
    write_le16(pos, static_cast<uint16_t>(key.size()));
    write_le16(pos + 2, static_cast<uint16_t>(val.size()));

    memcpy(&data[pos + 4], key.data(), key.size());
    memcpy(&data[pos + 4 + key.size()], val.data(), val.size());

    set_offset(idx + 1, get_offset(idx) + 4 + static_cast<uint16_t>(key.size() + val.size()));
}

void BNode::leaf_insert(const BNode& old, uint16_t idx, ByteVecView key, ByteVecView val) {
    set_header(BNODE_LEAF, old.nkeys() + 1);
    append_range(old, 0, 0, idx);
    append_kv(idx, 0, key, val);
    append_range(old, idx + 1, idx, old.nkeys() - idx);
}

void BNode::leaf_update(const BNode& old, uint16_t idx, ByteVecView key, ByteVecView val) {
    set_header(BNODE_LEAF, old.nkeys());
    append_range(old, 0, 0, idx);
    append_kv(idx, 0, key, val);
    append_range(old, idx + 1, idx + 1, old.nkeys() - (idx + 1));
}

void BNode::append_range(const BNode& old, uint16_t dst_new, uint16_t src_old, uint16_t n) {
    for (size_t i = 0; i < n; i++) {
        uint16_t dst = dst_new + i;
        uint16_t src = src_old + i;
        append_kv(dst, old.get_ptr(src), old.get_key(src), old.get_val(src));
    }
}

// Find the last position that is less than or equal to the key
uint16_t BNode::lookup_le_pos(ByteVecView key) const {
    uint16_t n_keys = nkeys();
    size_t i = 0;
    // TODO: Could possible be binary search 
    for (; i < n_keys; i++) {
        const ByteVecView this_key = get_key(i);
        int cmp = lex_cmp_byte_vecs(this_key, key);
        // 0 if a == b, -1 if a < b, and +1 if a > b.
        if (cmp == 0) return i; // Equal means this index is where they are <=
        if (cmp > 0) return i - 1; // a > b therefore the previous index is where it is <= 
    }

    return i - 1;
}

std::pair<BNode, BNode> BNode::split() const {
    if (nkeys() < 2) 
        throw std::out_of_range("Too little keys to split into two");

    // The idea is to find how many bytes to put into the left BNode while being within the page size
    size_t n_left = nkeys() / 2;
    while (true) {
        size_t left_bytes = 4 + 8 * n_left + 2 * n_left + get_offset(n_left);
        if (n_left == 0 || left_bytes <= BTREE_PAGE_SIZE)
            break;
        n_left--;    
    }
    if (n_left < 1)
        throw std::length_error("Cannot split when left BNode will be size 0");

    // Try to equalize the number of bytes for the right BNode
    while (true) {
        size_t left_bytes = 4 + 8 * n_left + 2 * n_left + get_offset(n_left);
        size_t right_bytes = nbytes() - left_bytes + 4;
        if (right_bytes <= BTREE_PAGE_SIZE)
            break;
        n_left++; 
    }
    if (n_left >= nkeys()) 
        throw std::length_error("Cannot split when right BNode will be size 0");

    size_t n_right = nkeys() - n_left;

    BNode left;
    left.set_header(btype(), n_left);
    left.append_range(*this, 0, 0, n_left);
    BNode right;
    right.set_header(btype(), n_right);
    right.append_range(*this, 0, n_left, n_right);

    if (right.nbytes() > BTREE_PAGE_SIZE) 
        throw std::length_error("Right BNode bytes size exceed max BTREE_PAGE_SIZE");

    return std::pair<BNode, BNode> { left, right };
}
