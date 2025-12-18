#pragma once

#include "../shared/bytevecview.h"
#include "../btree/btree.h"
#include <filesystem>
#include <memory>
#include <vector>


struct KV {
public:
  std::filesystem::path m_Path;
  std::unique_ptr<BTree> m_Tree;
  int m_Fd;

  struct {
    int total;
    std::vector<std::vector<uint8_t>> chunks;
  } m_mmap;

  // Constructor
  explicit KV();
  // Copy constructor
  KV(const KV& other) = delete;
  // Destructor
  ~KV() = default;
  // Copy assignment constructor
  KV& operator=(const KV& other) = delete;
  // Move constructor
  KV(KV&& other) noexcept = default;
  // Move assignment 
  KV& operator=(KV&& other) noexcept = default;

  void open();
  void get(ByteVecView key);
  void set(ByteVecView key, ByteVecView val);
  bool del(ByteVecView key);



};