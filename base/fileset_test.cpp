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

TEST(FileSetTest, DiskEntry) {
  FileSet *fs = nullptr;
  ASSERT_TRUE(FileSet::Builder().AddDiskFile("base/testdata/a.txt").Build(&fs));
  EXPECT_EQ(1, fs->Size());

  File *file = fs->Get(0);
  EXPECT_EQ(1, file->Size());
  EXPECT_EQ('a', file->At(0));
  EXPECT_EQ("base/testdata", file->Dirname());
  EXPECT_EQ("a.txt", file->Basename());

  delete fs;
}

TEST(FileSetTest, MultiEntry) {
  FileSet *fs = nullptr;
  ASSERT_TRUE(FileSet::Builder()
                  .AddStringFile("base/testdata/b.txt", "b")
                  .AddDiskFile("base/testdata/a.txt")
                  .Build(&fs));
  EXPECT_EQ(2, fs->Size());

  File *file0 = fs->Get(0);
  EXPECT_EQ(1, file0->Size());
  EXPECT_EQ('a', file0->At(0));
  EXPECT_EQ("base/testdata", file0->Dirname());
  EXPECT_EQ("a.txt", file0->Basename());

  File *file1 = fs->Get(1);
  EXPECT_EQ(1, file1->Size());
  EXPECT_EQ('b', file1->At(0));
  EXPECT_EQ("base/testdata", file1->Dirname());
  EXPECT_EQ("b.txt", file1->Basename());

  delete fs;
}

}  // namespace base
