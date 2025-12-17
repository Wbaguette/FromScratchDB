#pragma once

#include "c.h"
#include "../utils/bytell_hash_map.hpp"
#include "btree.h"
#include <memory>
#include <string>
#include <cstdint>
#include <string_view>

struct C {
  public:
    std::unique_ptr<BTree> m_Tree;
    std::unique_ptr<ska::bytell_hash_map<std::string, std::string>> m_Ref;
    std::unique_ptr<ska::bytell_hash_map<uint64_t, BNode>> m_Pages;

    explicit C();
    // Copy constructor
    C(const C& other) = delete;
    // Destructor
    ~C() = default;
    // Copy assignment constructor
    C& operator=(const C& other) = delete;
    // Move constructor
    C(C&& other) noexcept = default;
    // Move assignment 
    C& operator=(C&& other) noexcept = default;
    void add(std::string_view key, std::string_view val);

  private:
    ByteVecView get_page(uint64_t ptr);
    uint64_t alloc_page(ByteVecView node);
    void del_page(uint64_t ptr);
};