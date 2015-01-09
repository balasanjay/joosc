#ifndef BASE_FILE_IMPL_H
#define BASE_FILE_IMPL_H

#include "base/errorlist.h"
#include "base/file.h"
#include "third_party/gtest/gtest.h"

namespace base {

class StringFileTest;

class StringFile final : public File {
 public:
  StringFile(string path, string content);
  ~StringFile() override;

 protected:
  FRIEND_TEST(StringFileTest, Simple);
};

class DiskFile final : public File {
 public:
  ~DiskFile() override;
  static bool LoadFile(string path, File** file_out, ErrorList* error_out);

 protected:
  FRIEND_TEST(DiskFileTest, Simple);
  DiskFile(string path, u8* buf, int len) : File(path, buf, len) {}
};

}  // namespace base

#endif
