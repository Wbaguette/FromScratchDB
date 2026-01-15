#include "freelist.h"

#include <cstdint>
#include <cstring>
#include <vector>

LNode::LNode(ByteVecView data)
    : m_Data(const_cast<uint8_t*>(data.data())), m_Size(static_cast<size_t>(data.size())) {}

LNode::LNode(MutableByteVecView data)
    : m_Data(data.data()), m_Size(static_cast<size_t>(data.size())) {}

uint64_t LNode::get_next() const {
    uint64_t v;
    std::memcpy(&v, m_Data, sizeof(v));
    return v;
}

void LNode::set_next(uint64_t next) const { std::memcpy(m_Data, &next, sizeof(next)); }

uint64_t LNode::get_ptr(size_t idx) const {
    if (idx >= FREE_LIST_CAP) {
        return 0;
    }
    uint64_t v;
    size_t offset = FREE_LIST_HEADER + (idx * sizeof(uint64_t));
    if (offset + sizeof(uint64_t) > m_Size) {
        return 0;
    }
    std::memcpy(&v, m_Data + offset, sizeof(v));
    return v;
}

void LNode::set_ptr(size_t idx, uint64_t ptr) const {
    if (idx >= FREE_LIST_CAP) {
        return;
    }
    size_t offset = FREE_LIST_HEADER + (idx * sizeof(uint64_t));
    if (offset + sizeof(uint64_t) > m_Size) {
        return;
    }
    std::memcpy(m_Data + offset, &ptr, sizeof(ptr));
}

FreeList::FreeList() : head_page(0), head_seq(0), tail_page(0), tail_seq(0), max_seq(0) {}

uint64_t FreeList::pop_head() {
    auto [ptr, head] = fl_pop(*this);
    if (head != 0) {
        push_tail(head);
    }

    return ptr;
}

void FreeList::push_tail(uint64_t ptr) {
    LNode(m_Callbacks.set(tail_page)).set_ptr(seq2idx(tail_seq), ptr);
    tail_seq++;

    if (seq2idx(tail_seq) == 0) {
        auto [next, head] = fl_pop(*this);
        if (next == 0) {
            next = m_Callbacks.alloc(std::vector<uint8_t>(BTREE_PAGE_SIZE));
        }

        LNode(m_Callbacks.set(tail_page)).set_next(next);

        tail_page = next;

        if (head != 0) {
            LNode(m_Callbacks.set(tail_page)).set_ptr(0, head);
            tail_seq++;
        }
    }
}

std::pair<uint64_t, uint64_t> fl_pop(FreeList& fl) {
    if (fl.head_seq == fl.max_seq) {
        return {0, 0};
    }

    auto node = LNode(fl.m_Callbacks.get(fl.head_page));
    uint64_t ptr = node.get_ptr(seq2idx(fl.head_seq));
    fl.head_seq++;

    if (seq2idx(fl.head_seq) == 0) {
        uint64_t head = fl.head_page;
        fl.head_page = node.get_next();

        return {ptr, head};
    }

    return {ptr, 0};
}
