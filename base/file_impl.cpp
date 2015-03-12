#include "base/error.h"
#include "base/errorlist.h"
#include "base/file_impl.h"
#include "base/macros.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace base {

namespace internal {

class DiskFileError : public base::Error {
 public:
  DiskFileError(int errval, const string& path)
      : errval_(errval), path_(path) {}

  void PrintTo(std::ostream* out,
               const base::OutputOptions& opt, const base::FileSet*) const override {
    if (opt.simple) {
      *out << "DiskFileError{" << FIELD_PRINT(errval_) << FIELD_PRINT(path_)
           << "}";
      return;
    }

    *out << path_ << " " << opt.Red() << "error: " << opt.ResetColor()
         << string(strerror(errval_));
  }

 private:
  int errval_;
  string path_;
};

u8* StringCopy(string s) {
  u8* buf = new u8[s.length()];
  memmove(buf, s.c_str(), s.length());
  return buf;
}

}  // namespace internal

StringFile::StringFile(string path, string content)
    : File(path, internal::StringCopy(content), content.length()) {}

StringFile::~StringFile() { delete[] buf_; }

bool DiskFile::LoadFile(string path, File** file_out, ErrorList* error_out) {
  CHECK(file_out != nullptr);
  CHECK(error_out != nullptr);

  RESET_ERRNO;
  int fd = open(path.c_str(), O_RDONLY);
  if (fd == -1) {
    error_out->Append(new internal::DiskFileError(errno, path));
    return false;
  }

  RESET_ERRNO;
  struct stat filestat;
  if (fstat(fd, &filestat) == -1) {
    error_out->Append(new internal::DiskFileError(errno, path));
    close(fd);
    return false;
  }

  RESET_ERRNO;
  void* addr = mmap(nullptr, filestat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED) {
    error_out->Append(new internal::DiskFileError(errno, path));
    close(fd);
    return false;
  }
  close(fd);

  *file_out = new DiskFile(path, (u8*)addr, filestat.st_size);
  return true;
}

DiskFile::~DiskFile() { munmap((void*)buf_, len_); }

}  // namespace base
