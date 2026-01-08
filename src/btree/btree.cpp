#include "btree.h"

#include <sys/_types/_key_t.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../shared/treesizes.h"
#include "../utils/bytes.h"
#include "bnode.h"

BTree::BTree(size_t root) : m_Root(root) {}
BTree::BTree(Callbacks cbks) : m_Callbacks(std::move(cbks)) {}

void BTree::insert(ByteVecView key, ByteVecView val) {
    if (key.size() == 0) {
        throw std::runtime_error("key is empty");
    }
    if (key.size() > BTREE_MAX_KEY_SIZE) {
        throw std::runtime_error("key's size is greater than max allowed");
    }
    if (val.size() > BTREE_MAX_VAL_SIZE) {
        throw std::runtime_error("key's size is greater than max allowed");
    }

    if (m_Root == 0) {
        BNode root(BTREE_PAGE_SIZE);
        root.set_header(BNODE_LEAF, 2);

        node_append_kv(root, 0, 0, {}, {});
        node_append_kv(root, 1, 0, key, val);

        m_Root = static_cast<size_t>(m_Callbacks.alloc(root.m_Data));
        return;
    }

    BNode node = tree_insert(*this, m_Callbacks.get(m_Root), key, val);
    std::vector<BNode> res = try_split_thrice(node);
    m_Callbacks.del(static_cast<uint64_t>(m_Root));

    if (res.size() > 1) {
        BNode root(BTREE_PAGE_SIZE);
        root.set_header(BNODE_NODE, res.size());
        for (size_t i = 0; i < res.size(); i++) {
            uint64_t ptr = m_Callbacks.alloc(res[i].m_Data);
            ByteVecView key = res[i].get_key(0);
            node_append_kv(root, static_cast<uint16_t>(i), ptr, key, {});
        }
        m_Root = static_cast<size_t>(m_Callbacks.alloc(root.m_Data));
    } else {
        m_Root = static_cast<size_t>(m_Callbacks.alloc(res[0].m_Data));
    }
}

bool BTree::remove(ByteVecView key) {
    if (key.size() == 0) {
        throw std::length_error("Key to remove has size 0");
    }
    if (key.size() > BTREE_MAX_KEY_SIZE) {
        throw std::length_error("Key to remove has size greater than max allowed");
    }

    BNode updated = tree_delete_key(*this, m_Callbacks.get(m_Root), key);
    if (updated.m_Data.size() == 0) {
        return false;
    }

    m_Callbacks.del(m_Root);
    if (updated.btype() == static_cast<uint16_t>(BNODE_NODE) && updated.nkeys() == 1) {
        m_Root = static_cast<size_t>(updated.get_ptr(0));
    } else {
        m_Root = static_cast<size_t>(m_Callbacks.alloc(updated.m_Data));
    }

    return true;
}

std::vector<uint8_t> BTree::get(ByteVecView key) const {
    if (key.size() == 0) {
        return {};
    }
    if (key.size() > BTREE_MAX_KEY_SIZE) {
        return {};
    }

    if (m_Root == 0) {
        return {};
    }

    BNode node(m_Callbacks.get(m_Root));
    while (true) {
        uint16_t idx = lookup_le_pos(node, key);

        if (node.btype() == BNODE_LEAF) {
            if (lex_cmp_byte_vecs(key, node.get_key(idx)) == 0) {
                ByteVecView val = node.get_val(idx);
                return {val.begin(), val.end()};
            }
            return {};
        }

        if (node.btype() == BNODE_NODE) {
            uint64_t ptr = node.get_ptr(idx);
            node = BNode(m_Callbacks.get(ptr));
        } else {
            return {};
        }
    }
}

BNode tree_insert(BTree& tree, const BNode& node, ByteVecView key, ByteVecView val) {
    BNode new_(static_cast<size_t>(2 * BTREE_PAGE_SIZE));

    uint16_t idx = lookup_le_pos(node, key);
    switch (node.btype()) {
        case BNODE_LEAF: {
            if (lex_cmp_byte_vecs(key, node.get_key(idx)) == 0) {
                leaf_update(new_, node, idx, key, val);
            } else {
                leaf_insert(new_, node, idx + 1, key, val);
            }
            break;
        }
        case BNODE_NODE: {
            uint64_t k_ptr = node.get_ptr(idx);
            BNode temp(tree.m_Callbacks.get(k_ptr));
            BNode k_node = tree_insert(tree, temp, key, val);

            std::vector<BNode> res = try_split_thrice(k_node);
            tree.m_Callbacks.del(k_ptr);

            node_replace_kid_n(tree, new_, node, idx, res);
            break;
        }
        default: {
            throw std::runtime_error(
                "Switch on node btype gave value other than predefined consts");
        }
    }

    return new_;
}

void node_replace_kid_n(BTree& tree, BNode& new_, const BNode& old, uint16_t idx,
                        std::span<const BNode> kids) {
    auto inc = static_cast<uint16_t>(kids.size());
    new_.set_header(BNODE_NODE, old.nkeys() + inc - 1);
    node_append_range(new_, old, 0, 0, idx);
    for (size_t i = 0; i < inc; i++) {
        node_append_kv(new_, idx + static_cast<uint16_t>(i), tree.m_Callbacks.alloc(kids[i].m_Data),
                       kids[i].get_key(0), {});
    }
    node_append_range(new_, old, idx + inc, idx + 1, old.nkeys() - (idx + 1));
}

void node_merge(BNode& new_, BNode& left, BNode& right) {
    new_.set_header(left.btype(), left.nkeys() + right.nkeys());
    node_append_range(new_, left, 0, 0, left.nkeys());
    node_append_range(new_, right, left.nkeys(), 0, right.nkeys());
}

void leaf_delete_key(BNode& new_, const BNode& old, uint16_t idx) {
    new_.set_header(BNODE_LEAF, old.nkeys() - 1);
    node_append_range(new_, old, 0, 0, idx);
    node_append_range(new_, old, idx, idx + 1, old.nkeys() - (idx + 1));
}

BNode tree_delete_key(BTree& tree, const BNode& node, ByteVecView key) {
    uint16_t idx = lookup_le_pos(node, key);

    switch (node.btype()) {
        case BNODE_LEAF: {
            if (lex_cmp_byte_vecs(key, node.get_key(idx)) != 0) {
                return BNode{};
            }

            BNode new_(BTREE_PAGE_SIZE);
            leaf_delete_key(new_, node, idx);

            return new_;
        }
        case BNODE_NODE: {
            return node_delete_key(tree, node, idx, key);
        }
        default: {
            throw std::runtime_error(
                "Switch on node btype gave value other than predefined consts");
        }
    }
}

std::pair<int8_t, BNode> should_merge(BTree& tree, const BNode& node, uint16_t idx,
                                      const BNode& updated) {
    if (updated.nbytes() > static_cast<uint16_t>(BTREE_PAGE_SIZE) / 4) {
        return {0, BNode{}};
    }

    if (idx > 0) {
        BNode sibling(tree.m_Callbacks.get(node.get_ptr(idx - 1)));
        uint16_t merged = sibling.nbytes() + updated.nbytes() - static_cast<uint16_t>(HEADER);
        if (merged <= static_cast<uint16_t>(BTREE_PAGE_SIZE)) {
            return {-1, sibling};
        }
    }

    if (idx + 1 < node.nkeys()) {
        BNode sibling(tree.m_Callbacks.get(node.get_ptr(idx + 1)));
        uint16_t merged = sibling.nbytes() + updated.nbytes() - static_cast<uint16_t>(HEADER);
        if (merged <= static_cast<uint16_t>(BTREE_PAGE_SIZE)) {
            return {1, sibling};
        }
    }

    return {0, BNode{}};
}

void node_replace_2_kid(BNode& new_, const BNode& old, uint16_t idx, uint64_t merged,
                        ByteVecView key) {
    new_.set_header(BNODE_NODE, old.nkeys() - 1);
    node_append_range(new_, old, 0, 0, idx);
    node_append_kv(new_, idx, merged, key, {});
    node_append_range(new_, old, idx + 1, idx + 2, old.nkeys() - (idx + 1));
}

BNode node_delete_key(BTree& tree, const BNode& node, uint16_t idx, ByteVecView key) {
    uint64_t k_ptr = node.get_ptr(idx);
    BNode updated = tree_delete_key(tree, tree.m_Callbacks.get(k_ptr), key);
    if (updated.m_Data.size() == 0) {
        return BNode{};
    }

    tree.m_Callbacks.del(k_ptr);
    BNode new_(BTREE_PAGE_SIZE);
    auto [merge_dir, sibling] = should_merge(tree, node, idx, updated);
    switch (merge_dir) {
        case -1: {
            BNode merged(BTREE_PAGE_SIZE);
            node_merge(merged, sibling, updated);
            tree.m_Callbacks.del(node.get_ptr(idx - 1));
            node_replace_2_kid(new_, node, idx - 1, tree.m_Callbacks.alloc(merged.m_Data),
                               merged.get_key(0));
            break;
        }
        case 1: {
            BNode merged(BTREE_PAGE_SIZE);
            node_merge(merged, sibling, updated);
            tree.m_Callbacks.del(node.get_ptr(idx + 1));
            node_replace_2_kid(new_, node, idx - 1, tree.m_Callbacks.alloc(merged.m_Data),
                               merged.get_key(0));
            break;
        }
        case 0: {
            if (updated.nkeys() == 0) {
                if (node.nkeys() != 0 || idx != 0) {
                    throw std::runtime_error("Node num keys not 0 or idx is not 0");
                }
                new_.set_header(BNODE_NODE, 0);
            } else {
                node_replace_kid_n(tree, new_, node, idx, std::vector<BNode>{updated});
            }
            break;
        }
        default: {
            throw std::runtime_error("Switch on merge dir gave value other than -1, 1, 0");
        }
    }

    return new_;
}
