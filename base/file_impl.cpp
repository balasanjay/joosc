#include "base/file_impl.h"
#include <cstring>
#include <cerrno>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace base {

u8* StringCopy(std::string s) {
  u8* buf = new u8[s.length()];
  memmove(buf, s.c_str(), s.length());
  return buf;
}

StringFile::StringFile(std::string s) : File(StringCopy(s), s.length()) {}

StringFile::~StringFile() { delete[] buf_; }

bool DiskFile::LoadFile(std::string filename, File** out) {
  assert(out != nullptr);
  int fd = open(filename.c_str(), O_RDONLY);
  if (fd == -1) {
    return false;
  }
  struct stat filestat;
  if (fstat(fd, &filestat) == -1) {
    close(fd);
    return false;
  }

  void* addr = mmap(nullptr, filestat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED) {
    close(fd);
    return false;
  }
  close(fd);
  *out = new DiskFile((u8*)addr, filestat.st_size);
  return true;
}

DiskFile::DiskFile(u8* buf, int len) : File(buf, len) {}

DiskFile::~DiskFile() { munmap((void*)buf_, len_); }

}  // namespace base
