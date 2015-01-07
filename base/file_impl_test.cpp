#include "base/file.h"
#include "base/file_impl.h"
#include "third_party/gtest/gtest.h"
#include <string>

namespace base {

void ValidateFile(const File& f, i64 offset, const std::string& expected) {
  for (i64 i = offset; i < offset + (i64)expected.length(); ++i) {
    EXPECT_EQ(expected[i], f[i]);
  }
}

TEST(StringFileTest, Simple) {
  std::string text = "hello";
  StringFile sf(text);
  EXPECT_EQ(5, sf.length());
  ValidateFile(sf, 0, text);
}

TEST(StringFileTest, TestReadOutsideBounds) {
  StringFile sf("foo");
  ASSERT_THROW({
    sf[10];
  }, std::string);
}

TEST(DiskFileTest, Simple) {
  File* df = nullptr;
  ASSERT_TRUE(DiskFile::LoadFile("./testdata/testfile.txt", &df));
  EXPECT_EQ(11, df->length());
  ValidateFile(*df, 0, "test file\n");
}

} // namespace base

