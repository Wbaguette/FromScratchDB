#include "c.h"
#include "btree.h"
#include <memory>

C::C():
  m_Tree([] {
    return std::make_unique<BTree>(0);
  }()),
  m_Ref([] {
    return std::make_unique<ska::bytell_hash_map<std::string, std::string>>();
  }()),
  m_Pages([] {
    return std::make_unique<ska::bytell_hash_map<uint64_t, BNode>>();
  }())
{
  

}