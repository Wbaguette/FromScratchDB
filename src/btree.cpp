#include "btree.h"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

Node::Node() {
    static_assert(4 + 8 + 2 + 4 + BTREE_MAX_KEY_SIZE + BTREE_MAX_VAL_SIZE <= BTREE_PAGE_SIZE, "Node size is greater than max btree page size");
}

std::vector<uint8_t> Node::encode() {}

std::unique_ptr<Node> Node::decode(std::vector<uint8_t> page) {}





BNode::BNode(size_t size): data(size) {} 

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

void BNode::set_header(uint16_t btype, uint16_t nkeys) {
    write_le16(0, btype);
    write_le16(2, nkeys);
}

uint64_t BNode::get_ptr(uint16_t idx) const {
    if (idx < nkeys()) 
        throw std::out_of_range("child pointer idx is greater than number of keys");

    size_t pos = 4 + 8 * idx; // Position of child pointer is 4 bytes (header) + 8 * position (each child pointer is 8 bytes on 64bit arch)
    return read_le16(pos);
}

void BNode::set_ptr(uint16_t idx, uint64_t val) {
    if (idx < nkeys()) 
        throw std::out_of_range("child pointer idx is greater than number of keys");

    size_t pos = 4 + 8 * idx; // Position of child pointer is 4 bytes (header) + 8 * position (each child pointer is 8 bytes on 64bit arch)
    return write_le16(pos, val);
}

uint16_t BNode::get_offset(uint16_t idx) {
    if (idx == 0) 
        return 0;

    size_t pos = 4 + 8 * nkeys() + 2 * (idx - 1);
    return read_le16(pos);
}

uint16_t BNode::kv_pos(uint16_t idx) {
    if (idx < nkeys()) 
        throw std::out_of_range("kv idx is greater than number of keys");

    return 4 + 8 * nkeys() + 2 * nkeys() + get_offset(idx);
}

std::vector<uint8_t> BNode::get_key(uint16_t idx) {
    if (idx < nkeys()) 
        throw std::out_of_range("key index is greater than number of keys");

    size_t pos = kv_pos(idx);
    uint16_t key_length = read_le16(data[pos]);
    
    return std::vector<uint8_t>(data.begin() + pos + 4, data.begin() + key_length + 1);
}

std::vector<uint8_t> BNode::get_val(uint16_t idx) {
    if (idx < nkeys()) 
        throw std::out_of_range("val index is greater than number of keys");

    size_t pos = kv_pos(idx);
    uint16_t key_length = read_le16(data[pos]);
    uint16_t val_length = read_le16(data[pos + 2]);

    return std::vector<uint8_t>(data.begin() + pos + 4 + key_length, data.begin() + val_length + 1);
}



uint16_t BNode::read_le16(size_t offset) const {
    return static_cast<uint16_t>(data[offset]) | (static_cast<uint16_t>(data[offset + 1]) << 8);
}

void BNode::write_le16(size_t offset, uint16_t val) {
    if (data.size() < offset + 2)
        data.resize(offset + 2);

    data[offset] = static_cast<uint8_t>(val & 0xFF);
    data[offset + 1] = static_cast<uint8_t>((val >> 8) & 0xFF);
}