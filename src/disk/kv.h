#pragma once

#include "../shared/bytevecview.h"
#include "../btree/btree.h"
#include <filesystem>
#include <memory>


struct KV {
public:
  std::filesystem::path m_Path;
  std::unique_ptr<BTree> m_Tree;
  int m_Fd;
  bool failed;

  struct {
    int total;
    MmapView chunks;
  } m_Mmap;

  struct {
    uint64_t flushed;
    PageView temp;
  } m_Page;

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

  ByteVecView page_read(uint64_t ptr);
  uint64_t page_append(ByteVecView node_data);

};

ByteVecView save_meta(KV& db);
void load_meta(KV& db, ByteVecView data);
void read_root(KV& db, size_t file_size);
void update_root(KV& db);
void write_pages(KV& db);
int update_file(KV& db);
int create_file_sync(const std::filesystem::path& file);
void extend_mmap(KV& db, int size);
