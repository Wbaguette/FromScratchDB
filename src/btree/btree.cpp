#include "btree.h"
#include "../shared/treesizes.h"
#include "../utils/bytes.h"
#include "bnode.h"
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

Node::Node() {
    static_assert(4 + 8 + 2 + 4 + BTREE_MAX_KEY_SIZE + BTREE_MAX_VAL_SIZE <= BTREE_PAGE_SIZE, "Node size is greater than max btree page size");
}

std::vector<uint8_t> Node::encode() {}

std::unique_ptr<Node> Node::decode(std::vector<uint8_t> page) {}



BTree::BTree(size_t root): m_Root(root) {} 

BNode BTree::insert_node(const BNode& node, ByteVecView key, ByteVecView val) {
    BNode new_(2 * BTREE_PAGE_SIZE);

    uint16_t idx = node.lookup_le_pos(key);
    switch (node.btype()) {
        case BNODE_LEAF:
            if (lex_cmp_byte_vecs(key, node.get_key(idx)) == 0) {
                new_.leaf_update(node, idx, key, val);
            } else {
                new_.leaf_insert(node, idx, key, val);
            }
            break;
        case BNODE_NODE:
            uint64_t k_ptr = node.get_ptr(idx);
            // BNode k_node = insert_node(const BNode &node, ByteVecView key, ByteVecView val)

            break;
        default:
            throw std::runtime_error("Switch on node btype gave value other than predefined consts");
    }

    return new_;
}
