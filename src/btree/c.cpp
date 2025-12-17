#include "c.h"
#include "btree.h"
#include "../utils/bytes.h"
#include <memory>
#include <stdexcept>
#include <string_view>

C::C():
  m_Ref([] {
    return std::make_unique<ska::bytell_hash_map<std::string, std::string>>();
  }()),
  m_Pages([] {
    return std::make_unique<ska::bytell_hash_map<uint64_t, BNode>>();
  }())
{
  BTree::Callbacks cbks {
    [this](uint64_t ptr) { return this->get_page(ptr); },
    [this](ByteVecView node) { return this->alloc_page(node); },
    [this](uint64_t ptr) { return this->del_page(ptr); }
  }; 

  m_Tree = std::make_unique<BTree>(std::move(cbks));
}

void C::add(std::string_view key, std::string_view val) {
  m_Tree->insert(str_to_byte_vec(key), str_to_byte_vec(val));
  auto _ = m_Ref->emplace(key, val);
}

ByteVecView C::get_page(uint64_t ptr) {
  auto found = m_Pages->find(ptr);
  if (found == m_Pages->end())
    throw std::runtime_error("Cannot find key in m_Pages");

  return found->second.m_Data;
}

uint64_t C::alloc_page(ByteVecView node) {
  BNode bnode = BNode(node);
  if (bnode.nbytes() > BTREE_PAGE_SIZE)
    throw std::length_error("Created bnode has greater size than max allowed");
  
  static std::atomic<uint64_t> next_id{1};
  uint64_t ptr = next_id.fetch_add(1, std::memory_order_relaxed);
  
  if (m_Pages->find(ptr) != m_Pages->end())
    throw std::runtime_error("Unlikely key collision occured");
  
  (*m_Pages)[ptr] = std::move(bnode);
  return ptr;
}

void C::del_page(uint64_t ptr) {
  auto found = m_Pages->find(ptr);
  if (found == m_Pages->end())
    throw std::runtime_error("Cannot find key in m_Pages");

  m_Pages->erase(found);
}