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
        case BNODE_LEAF: {
            if (lex_cmp_byte_vecs(key, node.get_key(idx)) == 0) {
                new_.leaf_update(node, idx, key, val);
            } else {
                new_.leaf_insert(node, idx, key, val);
            }
            break;
        }
        case BNODE_NODE: {
            uint64_t k_ptr = node.get_ptr(idx);
            BNode temp(get(k_ptr));
            BNode k_node = insert_node(temp, key, val);
            
            std::span<const BNode> res = k_node.try_split_thrice();
            del(k_ptr);

            node_replace_kid_n(new_, node, idx, res);
            break;
        }
        default: {
            throw std::runtime_error("Switch on node btype gave value other than predefined consts");
        }
    }

    return new_;
}

void BTree::node_replace_kid_n(BNode& new_, const BNode& old, uint16_t idx, std::span<const BNode> kids) {
    uint16_t inc = kids.size();
    new_.set_header(BNODE_NODE, old.nkeys() + inc - 1);
    new_.append_range(old, 0, 0, idx);
    for (size_t i = 0; i < inc; i++) {
        new_.append_kv(idx + static_cast<uint16_t>(i), alloc(kids[i].m_Data) , kids[i].get_key(0), {});
    }
    new_.append_range(old, idx + inc, idx + 1, old.nkeys() - (idx + 1));
}

void BTree::insert(ByteVecView key, ByteVecView val) {
    if (key.size() == 0) 
        throw std::runtime_error("key is empty");
    if (key.size() > BTREE_MAX_KEY_SIZE) 
        throw std::runtime_error("key's size is greater than max allowed");
    if (val.size() > BTREE_MAX_VAL_SIZE) 
        throw std::runtime_error("key's size is greater than max allowed");

    if (m_Root == 0) {
        BNode root(BTREE_PAGE_SIZE);
        root.set_header(BNODE_LEAF, 2);

        root.append_kv(0, 0, {}, {});
        root.append_kv(1, 0, key, val);
        m_Root = alloc(root.m_Data);
        return;
    }
    
    BNode node = insert_node(get(m_Root), key, val);
    std::span<const BNode> res = node.try_split_thrice();
    del(m_Root);

    if (res.size() > 1) {
        BNode root(BTREE_PAGE_SIZE);
        root.set_header(BNODE_NODE, res.size());
        for (size_t i = 0; i < res.size(); i++) {
            uint64_t ptr = alloc(res[i].m_Data);
            ByteVecView key = res[i].get_key(0);
            root.append_kv(static_cast<uint16_t>(i), ptr, key, {});
        }
        m_Root = alloc(root.m_Data);
    } else {
        m_Root = alloc(res[0].m_Data);
    }
}
