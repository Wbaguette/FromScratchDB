#include "freelist.h"

#include <cstdint>
#include <vector>

LNode::LNode(ByteVecView data) { m_Data = std::vector<uint8_t>(data.begin(), data.end()); }

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

    LNode node = LNode(fl.m_Callbacks.get(fl.head_page));
    uint64_t ptr = node.get_ptr(seq2idx(fl.head_seq));
    fl.head_seq++;

    if (seq2idx(fl.head_seq) == 0) {
        uint64_t head = fl.head_page;
        fl.head_page = node.get_next();

        return {ptr, head};
    }

    return {ptr, 0};
}
