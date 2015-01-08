#include "base/fileset.h"
#include "third_party/gtest/gtest.h"

namespace base {

TEST(FileSetTest, EmptyBuilder) {
  FileSet *fs = nullptr;
  ASSERT_TRUE(FileSet::Builder().Build(&fs));
  EXPECT_EQ(0, fs->Size());
  delete fs;
}

TEST(FileSetTest, StringEntry) {
  FileSet *fs = nullptr;
  ASSERT_TRUE(FileSet::Builder().AddStringFile("a.txt", "a").Build(&fs));
  EXPECT_EQ(1, fs->Size());

  File *file = fs->Get(0);
  EXPECT_EQ(1, file->Size());
  EXPECT_EQ('a', file->At(0));
  EXPECT_EQ("", file->Dirname());
  EXPECT_EQ("a.txt", file->Basename());

  delete fs;
}

}  // namespace base
