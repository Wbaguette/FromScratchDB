#pragma once
#include <cstdint>
#include <vector>
#include "../shared/treesizes.h"
#include "../shared/views.h"
#include <functional>

constexpr int FREE_LIST_HEADER = 8;
constexpr int FREE_LIST_CAP = (BTREE_PAGE_SIZE - FREE_LIST_HEADER) /  8;

struct LNode {
  std::vector<uint8_t> m_Data;

   // Constructor
  explicit LNode(ByteVecView data);
  // Copy constructor
  LNode(const LNode& other) = delete;
  // Destructor
  ~LNode() = default;
  // Copy assignment constructor
  LNode& operator=(const LNode& other) = delete;
  // Move constructor
  LNode(LNode&& other) noexcept = default;
  // Move assignment 
  LNode& operator=(LNode&& other) noexcept = default;
  uint64_t get_next();
  void set_next(uint64_t next);
  uint64_t get_ptr(size_t idx);
  void set_ptr(size_t idx, uint64_t ptr);
};


struct FreeList {
  uint64_t head_page;
  uint64_t head_seq;
  uint64_t tail_page;
  uint64_t tail_seq;
  uint64_t max_seq;

  struct Callbacks {
    std::function<ByteVecView(uint64_t)> get;
    std::function<uint64_t(ByteVecView)> alloc;
    std::function<ByteVecView(uint64_t)> set;
  };
  Callbacks m_Callbacks;

  // Constructor
  explicit FreeList();
  // Copy constructor
  FreeList(const FreeList& other) = delete;
  // Destructor
  ~FreeList() = default;
  // Copy assignment constructor
  FreeList& operator=(const FreeList& other) = delete;
  // Move constructor
  FreeList(FreeList&& other) noexcept = default;
  // Move assignment 
  FreeList& operator=(FreeList&& other) noexcept = default;
  uint64_t pop_head();
  void push_tail(uint64_t ptr);
  void set_max_seq() {
    max_seq = tail_seq;
  }

};

std::pair<uint64_t, uint64_t> fl_pop(FreeList& fl);
inline int seq2idx(uint64_t seq) {
  return static_cast<int>(seq % FREE_LIST_CAP);
}

