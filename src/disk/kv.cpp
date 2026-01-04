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
#include <stdexcept>
#include <vector>

constexpr std::string DB_SIG = "bigchungus";

// | sig | root_ptr | page_used |
// | 16B |    8B    |     8B    |
std::array<uint8_t, 32> save_meta(KV& db) {
    std::array<uint8_t, 32> data;
    size_t len = std::min(static_cast<int>(DB_SIG.size()), 16);

    std::memcpy(data.data(), DB_SIG.c_str(), len);

    std::memcpy(&data[16], &db.m_Tree->m_Root, sizeof(db.m_Tree->m_Root));
    std::memcpy(&data[24], &db.m_Page.flushed, sizeof(db.m_Page.flushed));

    return data;
}

void load_meta(KV& db, ByteVecView data) {
    // TODO
}

void read_root(KV& db, size_t file_size) {
    if (file_size == 0) {
        db.m_Page.flushed = 1;
        return;
    }

    ByteVecView data = db.m_Mmap.chunks[0];
    load_meta(db, data);

    // TODO
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
    int size = (static_cast<int>(db.m_Page.flushed) + db.m_Page.temp.size()) * BTREE_PAGE_SIZE;
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

void update_or_revert(KV& db, ByteVecView meta) {
    if (db.failed) {
        // TODO
        db.failed = false;
    }

    int status = update_file(db);
    if (status == -1) {
        load_meta(db, meta);
        db.m_Page.temp.clear();
    }
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
        return res1;
    }
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

void extend_mmap(KV& db, int size) {
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
    db.m_Mmap.chunks.push_back(chunk_data);
}

KV::KV() {}

void KV::open() {
    m_Tree->m_Callbacks.get = [this](uint64_t ptr) { return page_read(ptr); };
    m_Tree->m_Callbacks.alloc = [this](ByteVecView node_data) { return page_alloc(node_data); };
    m_Tree->m_Callbacks.del = [this](uint64_t ptr) { m_FreeList->push_tail(ptr); };

    m_FreeList->m_Callbacks.get = [this](uint64_t ptr) { return page_read(ptr); };
    m_FreeList->m_Callbacks.alloc = [this](ByteVecView node_data) {
        return page_append(node_data);
    };
    m_FreeList->m_Callbacks.set = [this](uint64_t ptr) { return page_write(ptr); };
}

void KV::get(ByteVecView key) {
    // m_Tree->m_Callbacks.get(key);
}

void KV::set(ByteVecView key, ByteVecView val) {
    ByteVecView meta = save_meta(*this);
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
    for (size_t i = 0; i < m_Mmap.chunks.size(); i++) {
        uint64_t end = start + static_cast<uint64_t>(m_Mmap.chunks[i].size()) /
                                   static_cast<uint64_t>(BTREE_PAGE_SIZE);
        if (ptr < end) {
            uint64_t offset = static_cast<uint64_t>(BTREE_PAGE_SIZE) * (ptr - start);
            return std::vector<uint8_t>(m_Mmap.chunks[i].begin() + offset,
                                        m_Mmap.chunks[i].begin() + offset + BTREE_PAGE_SIZE);
        }
        start = end;
    }
    throw std::runtime_error("bad ptr");
}

uint64_t KV::page_append(ByteVecView node_data) {
    uint64_t ptr = m_Page.flushed + static_cast<uint64_t>(m_Page.temp.size());
    m_Page.temp.push_back(node_data);
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

    std::vector<uint8_t> node;
    node.reserve(BTREE_PAGE_SIZE);

    ByteVecView data = page_read_file(ptr);
    std::memcpy(node.data(), data.data(), data.size());
    m_Page.updates->emplace(ptr, node);

    return node;
}
