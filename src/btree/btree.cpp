#include "btree.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string_view>
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

uint16_t BNode::nbytes() {
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

uint16_t BNode::get_offset(uint16_t idx) {
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


uint16_t BNode::kv_pos(uint16_t idx) {
    if (idx > nkeys()) 
        throw std::out_of_range("kv idx is greater than number of keys");

    return 4 + 8 * nkeys() + 2 * nkeys() + get_offset(idx);
}

std::vector<uint8_t> BNode::get_key(uint16_t idx) {
    if (idx >= nkeys()) 
        throw std::out_of_range("key index is greater than number of keys");

    size_t pos = kv_pos(idx);
    uint16_t key_length = read_le16(pos);


    auto begin = data.begin() + pos + 4;
    auto last = begin + key_length;

    return std::vector<uint8_t>(begin, last);
}

std::vector<uint8_t> BNode::get_val(uint16_t idx) {
    if (idx >= nkeys()) 
        throw std::out_of_range("val index is greater than number of keys");

    size_t pos = kv_pos(idx);
    uint16_t key_length = read_le16(pos);
    uint16_t val_length = read_le16(pos + 2);

    auto begin = data.begin() + pos + 4 + key_length;
    auto last   = begin + val_length;

    return std::vector<uint8_t>(begin, last);
}

void BNode::append_kv(uint16_t idx, uint64_t ptr, const std::vector<uint8_t>& key, const std::vector<uint8_t>& val) {
    set_ptr(idx, ptr);
    size_t pos = kv_pos(idx);
    
    write_le16(pos, static_cast<uint16_t>(key.size()));
    write_le16(pos + 2, static_cast<uint16_t>(val.size()));

    memcpy(&data[pos + 4], key.data(), key.size());
    memcpy(&data[pos + 4 + key.size()], val.data(), val.size());

    set_offset(idx + 1, get_offset(idx) + 4 + static_cast<uint16_t>(key.size() + val.size()));
}