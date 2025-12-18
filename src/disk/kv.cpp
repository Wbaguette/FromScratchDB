#include "kv.h"
#include <stdexcept>
#include <sys/fcntl.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


void update_file(KV& db) {
  write_pages(db);

  int res1 = fsync(db.m_Fd);
  if (res1 == -1) 
    throw std::runtime_error("failed first fsync");

  update_root(db);

  int res2 = fsync(db.m_Fd);
  if (res2 == -1) 
    throw std::runtime_error("failed second fsync");
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



KV::KV() {}



void KV::open() {
  m_Tree->m_Callbacks.get = page_read;
  m_Tree->m_Callbacks.alloc = page_append;
  m_Tree->m_Callbacks.del = {};
}

void KV::get(ByteVecView key) {
  // m_Tree->m_Callbacks.get(key);

}

void KV::set(ByteVecView key, ByteVecView val) {
  m_Tree->insert(key, val);
  update_file(*this);
}

bool KV::del(ByteVecView key) {
  bool deleted = m_Tree->remove(key);
  update_file(*this);
  return deleted;
}

