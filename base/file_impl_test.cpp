#include "base/file.h"
#include "base/file_impl.h"
#include "third_party/gtest/gtest.h"

namespace base {

void ValidateFile(const File& f, i64 offset, const string& expected) {
  for (i64 i = offset; i < offset + (i64)expected.length(); ++i) {
    EXPECT_EQ(expected[i], f.At(i));
  }
}

TEST(StringFileTest, Simple) {
  string text = "hello";
  StringFile sf("hello.txt", text);
  EXPECT_EQ(5, sf.Size());
  ValidateFile(sf, 0, text);
}

TEST(StringFileTest, TestReadOutsideBounds) {
  StringFile sf("foo.txt", "foo");
  ASSERT_THROW({ sf.At(10); }, string);
}

TEST(DiskFileTest, Simple) {
  File* df = nullptr;
  ASSERT_TRUE(DiskFile::LoadFile("base/testdata/testfile.txt", &df));
  EXPECT_EQ(11, df->Size());
  ValidateFile(*df, 0, "test file\n");
  delete (DiskFile*)df;
}

}  // namespace base
