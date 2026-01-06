#pragma once

#include <fcntl.h>

#include <filesystem>
#include <memory>

#include "../btree/btree.h"
#include "../freelist/freelist.h"
#include "../shared/views.h"
#include "../utils/bytell_hash_map.hpp"

struct KV {
    std::filesystem::path m_Path;
    std::unique_ptr<BTree> m_Tree;
    std::unique_ptr<FreeList> m_FreeList;

    int m_Fd;
    bool failed;

    struct {
        int total;
        MmapView chunks;
    } m_Mmap;

    struct Page {
        uint64_t flushed;
        uint64_t napppend;
        std::unique_ptr<ska::bytell_hash_map<uint64_t, ByteVecView>> updates;
        PageView temp;

        explicit Page(std::unique_ptr<ska::bytell_hash_map<uint64_t, ByteVecView>> updates);
    } m_Page;

    // Constructor
    explicit KV(const std::string& path);
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

    void init();
    [[nodiscard]] std::vector<uint8_t> get(ByteVecView key) const;
    void set(ByteVecView key, ByteVecView val);
    bool del(ByteVecView key);

    ByteVecView page_read_file(uint64_t ptr);
    ByteVecView page_read(uint64_t ptr);
    uint64_t page_append(ByteVecView node_data);
    uint64_t page_alloc(ByteVecView node_data);
    std::vector<uint8_t> page_write(uint64_t ptr);
};

std::array<uint8_t, 32> save_meta(KV& db);
void load_meta(KV& db, ByteVecView data);
void read_root(KV& db, size_t file_size);
void update_root(KV& db);
void write_pages(KV& db);
int update_file(KV& db);
int create_file_sync(const std::filesystem::path& file);
void extend_mmap(KV& db, size_t size);
