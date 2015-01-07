#ifndef BASE_FILE_IMPL_H
#define BASE_FILE_IMPL_H

#include "base/file.h"
#include "third_party/gtest/gtest.h"
#include <string>
#include <sys/mman.h>

namespace base {

class StringFileTest;

class StringFile: public File {
public:
  StringFile(std::string s);
  ~StringFile() override;

protected:
  FRIEND_TEST(StringFileTest, Simple);
};

class DiskFile: public File {
public:
  ~DiskFile() override;
  static bool LoadFile(std::string filename, File** out);

protected:
  FRIEND_TEST(DiskFileTest, Simple);
  DiskFile(u8* buf, int len);
};

} // namespace base

#endif
