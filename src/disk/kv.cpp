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
#include <memory>
#include <stdexcept>
#include <vector>

#include "../shared/treesizes.h"

namespace {
void update_or_revert(KV& db, ByteVecView meta) {
    if (db.m_Failed) {
        db.m_Failed = false;
    }

    int status = update_file(db);

    if (status == -1) {
        load_meta(db, meta);
        db.m_Page.temp.clear();
        db.m_Page.updates->clear();
        db.m_Failed = true;
    }
}
}  // namespace

constexpr std::string DB_SIG = "bigchungus";

KV::Page::Page()
    : flushed(0),
      napppend(0),
      updates(std::make_unique<ska::bytell_hash_map<uint64_t, std::vector<uint8_t>>>()) {
    temp.reserve(1);
}

KV::Mmap::Mmap() : total(0) { chunks.reserve(BTREE_PAGE_SIZE); }

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
    std::memcpy(&db.m_Tree->m_Root, &data[16], sizeof(db.m_Tree->m_Root));
    std::memcpy(&db.m_Page.flushed, &data[24], sizeof(db.m_Page.flushed));
}

void read_root(KV& db, size_t file_size) {
    if (file_size == 0) {
        db.m_Page.flushed = 2;  // Reserve page 0 (metadata) and page 1 (freelist)
        db.m_FreeList->head_page = 1;
        db.m_FreeList->tail_page = 1;
        db.m_FreeList->head_seq = 0;
        db.m_FreeList->tail_seq = 0;
        db.m_FreeList->max_seq = 0;
        return;
    }

    ByteVecView data = db.m_Mmap.chunks[0];
    load_meta(db, data);

    // On restore, we must re-initialize the in-memory freelist state.
    // Page 0 is metadata; page 1 is reserved for freelist.
    // Without this, deletions will start writing the freelist into page 0, and subsequent
    // allocations may read bad pointers from metadata (showing up as huge uint64_t ptrs).
    db.m_FreeList->head_page = 1;
    db.m_FreeList->tail_page = 1;
    db.m_FreeList->head_seq = 0;
    db.m_FreeList->tail_seq = 0;
    db.m_FreeList->max_seq = 0;
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
    for (const auto& [ptr, page] : *db.m_Page.updates) {
        if (page.size() != BTREE_PAGE_SIZE) {
            throw std::runtime_error("update page has bad size");
        }

        if (ptr == 0) {
            throw std::runtime_error("attempted to write page 0 (metadata) as a btree page");
        }
        constexpr auto page_size = static_cast<uint64_t>(BTREE_PAGE_SIZE);
        const uint64_t max_ptr =
            static_cast<uint64_t>(std::numeric_limits<off_t>::max()) / page_size;
        if (ptr > max_ptr) {
            throw std::runtime_error("page ptr too large; would overflow file offset");
        }

        auto offset = static_cast<off_t>(ptr * page_size);

        ssize_t written = pwrite(db.m_Fd, page.data(), page.size(), offset);
        if (written == -1) {
            std::cerr << "pwrite failed: offset=" << offset << " errno=" << errno << " ("
                      << strerror(errno) << ")\n";
            throw std::runtime_error("failed pwrite");
        }
    }

    auto size = static_cast<size_t>((db.m_Page.flushed + db.m_Page.temp.size()) * BTREE_PAGE_SIZE);
    extend_mmap(db, size);

    if (!db.m_Page.temp.empty()) {
        std::vector<iovec> iov(db.m_Page.temp.size());
        for (size_t i = 0; i < db.m_Page.temp.size(); i++) {
            if (db.m_Page.temp[i].size() != BTREE_PAGE_SIZE) {
                throw std::runtime_error("temp page has bad size");
            }
            iov[i].iov_base = db.m_Page.temp[i].data();
            iov[i].iov_len = db.m_Page.temp[i].size();
        }

        auto offset = static_cast<off_t>(db.m_Page.flushed * BTREE_PAGE_SIZE);
        ssize_t written = pwritev(db.m_Fd, iov.data(), static_cast<int>(iov.size()), offset);
        if (written == -1) {
            std::cerr << "pwritev failed: offset=" << offset << " errno=" << errno << " ("
                      << strerror(errno) << ")\n";
            throw std::runtime_error("failed pwritev");
        }
    }

    db.m_Page.flushed += static_cast<uint64_t>(db.m_Page.temp.size());
    db.m_Page.temp.clear();
    db.m_Page.updates->clear();
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

    const auto dir = file.parent_path();
    const auto name = file.filename();

    int dirfd = open(dir.c_str(), O_RDONLY | O_DIRECTORY);
    if (dirfd == -1) {
        return -1;
    }

    int fd = openat(dirfd, name.c_str(), O_RDWR | O_CREAT, mode);
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

    size_t alloc = std::max(db.m_Mmap.total, static_cast<size_t>(1 << 20));
    while (db.m_Mmap.total + alloc < size) {
        alloc *= 2;
    }

    size_t new_file_size = db.m_Mmap.total + alloc;
    if (ftruncate(db.m_Fd, static_cast<off_t>(new_file_size)) == -1) {
        throw std::runtime_error("failed to extend file for mmap");
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

KV::KV(const std::string& path, bool restore_from_file)
    : m_Path(path),
      m_Tree(std::make_unique<BTree>(0)),
      m_FreeList(std::make_unique<FreeList>()),
      m_Fd(-1),
      m_Failed(false) {
    if (restore_from_file) {
        auto fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            throw std::runtime_error("Failed to open database file");
        }
        m_Fd = fd;

        struct stat st;
        fstat(m_Fd, &st);
        auto file_size = static_cast<size_t>(st.st_size);

        extend_mmap(*this, file_size);
        read_root(*this, st.st_size);
    } else {
        std::filesystem::remove(path);

        auto fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            throw std::runtime_error("Failed to open database file");
        }
        m_Fd = fd;

        read_root(*this, 0);

        std::vector<uint8_t> freelist_page(BTREE_PAGE_SIZE, 0);
        m_Page.temp.emplace_back(std::move(freelist_page));

        update_file(*this);

        struct stat st;
        fstat(m_Fd, &st);
        auto file_size = static_cast<size_t>(st.st_size);

        extend_mmap(*this, file_size);
    }

    init();
}

KV::~KV() {
    for (const auto& chunk : m_Mmap.chunks) {
        if (chunk.data() != nullptr && chunk.size() > 0) {
            munmap(const_cast<uint8_t*>(chunk.data()), chunk.size());
        }
    }
    m_Mmap.chunks.clear();
    m_Mmap.total = 0;

    if (m_Fd != -1) {
        close(m_Fd);
        m_Fd = -1;
    }
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

std::optional<std::string_view> KV::get(ByteVecView key) const { return m_Tree->get(key); }

void KV::set(ByteVecView key, ByteVecView val) {
    std::array<uint8_t, 32> meta = save_meta(*this);

    m_Tree->insert(key, val);
    update_or_revert(*this, meta);
}

bool KV::del(ByteVecView key) {
    if (!get(key).has_value()) {
        return false;
    }

    bool deleted = m_Tree->remove(key);
    update_file(*this);
    return deleted;
}

ByteVecView KV::page_read(uint64_t ptr) {
    if (auto found = m_Page.updates->find(ptr); found != m_Page.updates->end()) {
        const auto& page = found->second;
        return {page.data(), page.size()};
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

    std::vector<uint8_t> page(BTREE_PAGE_SIZE);
    std::memcpy(page.data(), node_data.data(),
                std::min(node_data.size(), static_cast<size_t>(BTREE_PAGE_SIZE)));
    m_Page.temp.emplace_back(std::move(page));
    return ptr;
}

uint64_t KV::page_alloc(ByteVecView node_data) {
    if (uint64_t ptr = m_FreeList->pop_head(); ptr != 0) {
        std::vector<uint8_t> page(BTREE_PAGE_SIZE);
        std::memcpy(page.data(), node_data.data(),
                    std::min(node_data.size(), static_cast<size_t>(BTREE_PAGE_SIZE)));
        m_Page.updates->emplace(ptr, std::move(page));
        return ptr;
    }

    return page_append(node_data);
}

MutableByteVecView KV::page_write(uint64_t ptr) {
    if (auto found = m_Page.updates->find(ptr); found != m_Page.updates->end()) {
        auto& page = found->second;
        return {page.data(), page.size()};
    }

    std::vector<uint8_t> page(BTREE_PAGE_SIZE);
    ByteVecView data = page_read_file(ptr);
    std::memcpy(page.data(), data.data(),
                std::min(data.size(), static_cast<size_t>(BTREE_PAGE_SIZE)));

    auto [it, inserted] = m_Page.updates->emplace(ptr, std::move(page));
    (void)inserted;
    auto& stored = it->second;
    return {stored.data(), stored.size()};
}
