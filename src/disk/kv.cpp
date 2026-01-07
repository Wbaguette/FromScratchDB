#include "kv.h"

#include <fcntl.h>
#include <sys/_types/_iovec_t.h>
#include <sys/_types/_off_t.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "../shared/treesizes.h"
#include "../utils/bytes.h"

namespace {
void update_or_revert(KV& db, ByteVecView meta) {
    if (db.failed) {
        db.failed = false;
    }

    int status = update_file(db);

    if (status == -1) {
        load_meta(db, meta);
        db.m_Page.temp.clear();
        db.m_Page.updates->clear();
        db.failed = true;
    }
}
}  // namespace

constexpr std::string DB_SIG = "bigchungus";

KV::Page::Page(std::unique_ptr<ska::bytell_hash_map<uint64_t, ByteVecView>> updates)
    : flushed(0), napppend(0), updates(std::move(updates)) {}

// | sig | root_ptr | page_used |
// | 16B |    8B    |     8B    |
std::array<uint8_t, 32> save_meta(KV& db) {
    std::array<uint8_t, 32> data;
    data.fill(0xFF);
    size_t len = std::min(static_cast<int>(DB_SIG.size()), 16);

    std::memcpy(data.data(), DB_SIG.c_str(), len);
    std::memcpy(data.data() + 16, &db.m_Tree->m_Root, sizeof(db.m_Tree->m_Root));
    std::memcpy(data.data() + 24, &db.m_Page.flushed, sizeof(db.m_Page.flushed));

    return data;
}

void load_meta(KV& db, ByteVecView data) {
    // Read root pointer from bytes 16-23
    std::memcpy(&db.m_Tree->m_Root, &data[16], sizeof(db.m_Tree->m_Root));
    // Read flushed page count from bytes 24-31
    std::memcpy(&db.m_Page.flushed, &data[24], sizeof(db.m_Page.flushed));
}

void read_root(KV& db, size_t file_size) {
    if (file_size == 0) {
        db.m_Page.flushed = 2;
        db.m_FreeList->head_page = 1;
        db.m_FreeList->tail_page = 1;
        db.m_FreeList->head_seq = 0;
        db.m_FreeList->tail_seq = 0;
        db.m_FreeList->max_seq = 0;
        return;
    }

    ByteVecView data = db.m_Mmap.chunks[0];
    load_meta(db, data);

    // TODO: Load freelist metadata from page 1 if needed
}

void update_root(KV& db) {
    auto m = save_meta(db);
    std::vector<uint8_t> meta(m.begin(), m.end());
    std::vector<iovec> iov(1);
    iov[0].iov_base = meta.data();
    iov[0].iov_len = meta.size();

    ssize_t written = pwritev(db.m_Fd, iov.data(), static_cast<int>(iov.size()), 0);
    if (written == -1) {
        throw std::runtime_error("failed pwritev");
    }
}

void write_pages(KV& db) {
    auto size = static_cast<size_t>((db.m_Page.flushed + db.m_Page.temp.size()) * BTREE_PAGE_SIZE);
    extend_mmap(db, size);

    std::vector<iovec> iov(db.m_Page.temp.size());
    for (size_t i = 0; i < db.m_Page.temp.size(); i++) {
        iov[i].iov_base = const_cast<uint8_t*>(db.m_Page.temp[i].data());
        iov[i].iov_len = db.m_Page.temp[i].size();
    }

    auto offset = static_cast<off_t>(db.m_Page.flushed * BTREE_PAGE_SIZE);
    ssize_t written = pwritev(db.m_Fd, iov.data(), static_cast<int>(iov.size()), offset);
    if (written == -1) {
        throw std::runtime_error("failed pwritev");
    }

    db.m_Page.flushed += static_cast<uint64_t>(db.m_Page.temp.size());
    db.m_Page.temp.clear();
}

int update_file(KV& db) {
    write_pages(db);

    int res1 = fsync(db.m_Fd);
    if (res1 == -1) {
        return res1;
    }

    update_root(db);

    int res2 = fsync(db.m_Fd);
    if (res2 == -1) {
        return res2;
    }

    db.m_FreeList->set_max_seq();
    return 0;
}

int create_file_sync(const std::filesystem::path& file) {
    mode_t mode = 0644;

    int flags = O_RDONLY | O_DIRECTORY;
    int dirfd = open(file.c_str(), flags, mode);
    if (dirfd == -1) {
        close(dirfd);
        return -1;
    }

    int flags2 = O_RDWR | O_CREAT;
    int fd = openat(dirfd, file.c_str(), flags, mode);
    if (fd == -1) {
        close(dirfd);
        return -1;
    }

    if (fsync(dirfd) == -1) {
        close(fd);
        return -1;
    }

    close(dirfd);

    return fd;
}

void extend_mmap(KV& db, size_t size) {
    if (size <= db.m_Mmap.total) {
        return;
    }

    int alloc = std::max(db.m_Mmap.total, 64 << 20);
    while (db.m_Mmap.total + alloc < size) {
        alloc *= 2;
    }

    void* chunk =
        mmap(nullptr, alloc, PROT_READ, MAP_SHARED, db.m_Fd, static_cast<off_t>(db.m_Mmap.total));

    if (chunk == MAP_FAILED) {
        throw std::runtime_error("failed mmap");
    }

    db.m_Mmap.total += alloc;

    ByteVecView chunk_data(static_cast<uint8_t*>(chunk), alloc);
    db.m_Mmap.chunks.emplace_back(chunk_data);
}

KV::KV(const std::string& path)
    : m_Path(path),
      m_Tree(std::make_unique<BTree>(0)),
      m_FreeList(std::make_unique<FreeList>()),
      m_Page{std::make_unique<ska::bytell_hash_map<uint64_t, ByteVecView>>()} {
    std::filesystem::remove(path);

    mode_t mode = 0644;
    int flags = O_RDWR | O_CREAT;
    auto fd = open(path.c_str(), flags, mode);
    if (fd == -1) {
        throw std::runtime_error("Failed to open database file");
    }

    m_Fd = fd;

    struct stat st;
    fstat(m_Fd, &st);
    if (st.st_size > 0) {
        extend_mmap(*this, static_cast<size_t>(st.st_size));
    }

    read_root(*this, static_cast<size_t>(st.st_size));
}

void KV::init() {
    m_Tree->m_Callbacks.get = [this](uint64_t ptr) { return page_read(ptr); };
    m_Tree->m_Callbacks.alloc = [this](ByteVecView node_data) { return page_alloc(node_data); };
    m_Tree->m_Callbacks.del = [this](uint64_t ptr) { m_FreeList->push_tail(ptr); };

    m_FreeList->m_Callbacks.get = [this](uint64_t ptr) { return page_read(ptr); };
    m_FreeList->m_Callbacks.alloc = [this](ByteVecView node_data) {
        return page_append(node_data);
    };
    m_FreeList->m_Callbacks.set = [this](uint64_t ptr) { return page_write(ptr); };
}

std::vector<uint8_t> KV::get(ByteVecView key) const { return m_Tree->get(key); }

void KV::set(ByteVecView key, ByteVecView val) {
    std::array<uint8_t, 32> meta = save_meta(*this);

    m_Tree->insert(key, val);
    update_or_revert(*this, meta);
}

bool KV::del(ByteVecView key) {
    bool deleted = m_Tree->remove(key);
    int status = update_file(*this);
    return deleted;
}

ByteVecView KV::page_read(uint64_t ptr) {
    if (auto found = m_Page.updates->find(ptr); found != m_Page.updates->end()) {
        return found->second;
    }

    return page_read_file(ptr);
}

ByteVecView KV::page_read_file(uint64_t ptr) {
    uint64_t start = 0;
    for (auto chunk : m_Mmap.chunks) {
        uint64_t end =
            start + (static_cast<uint64_t>(chunk.size()) / static_cast<uint64_t>(BTREE_PAGE_SIZE));
        if (ptr < end) {
            uint64_t page_index = ptr - start;
            uint64_t unsigned_offset = page_index * static_cast<uint64_t>(BTREE_PAGE_SIZE);

            if (unsigned_offset >
                static_cast<uint64_t>(std::numeric_limits<std::ptrdiff_t>::max())) {
                throw std::overflow_error("Offset too large for iterator arithmetic");
            }

            auto offset = static_cast<std::ptrdiff_t>(unsigned_offset);
            return {chunk.begin() + offset, chunk.begin() + offset + BTREE_PAGE_SIZE};
        }
        start = end;
    }
    throw std::runtime_error("bad ptr");
}

uint64_t KV::page_append(ByteVecView node_data) {
    uint64_t ptr = m_Page.flushed + static_cast<uint64_t>(m_Page.temp.size());
    m_Page.temp.emplace_back(node_data);
    return ptr;
}

uint64_t KV::page_alloc(ByteVecView node_data) {
    if (uint64_t ptr = m_FreeList->pop_head(); ptr != 0) {
        m_Page.updates->emplace(ptr, node_data);
        return ptr;
    }

    return page_append(node_data);
}

std::vector<uint8_t> KV::page_write(uint64_t ptr) {
    if (auto found = m_Page.updates->find(ptr); found != m_Page.updates->end()) {
        ByteVecView val = found->second;
        return {val.begin(), val.end()};
    }

    std::vector<uint8_t> node(BTREE_PAGE_SIZE);

    ByteVecView data = page_read_file(ptr);
    std::memcpy(node.data(), data.data(),
                std::min(data.size(), static_cast<size_t>(BTREE_PAGE_SIZE)));
    m_Page.updates->emplace(ptr, node);

    return node;
}
