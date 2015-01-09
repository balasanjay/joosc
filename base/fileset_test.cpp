#include "base/fileset.h"
#include "third_party/gtest/gtest.h"

namespace base {

class FileSetTest : public testing::Test {
protected:
  void SetUp() { fs = nullptr; }

  void TearDown() {
    delete fs;
    fs = nullptr;
  }

  ErrorList errors;
  FileSet *fs;
};

TEST_F(FileSetTest, EmptyBuilder) {
  ASSERT_TRUE(FileSet::Builder().Build(&fs, &errors));
  EXPECT_EQ(0, fs->Size());
  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(FileSetTest, StringEntry) {
  ASSERT_TRUE(FileSet::Builder().AddStringFile("a.txt", "a").Build(&fs, &errors));
  EXPECT_EQ(1, fs->Size());
  EXPECT_FALSE(errors.IsFatal());

  File *file = fs->Get(0);
  EXPECT_EQ(1, file->Size());
  EXPECT_EQ('a', file->At(0));
  EXPECT_EQ("", file->Dirname());
  EXPECT_EQ("a.txt", file->Basename());
}

TEST_F(FileSetTest, DiskEntry) {
  ASSERT_TRUE(FileSet::Builder().AddDiskFile("base/testdata/a.txt").Build(&fs, &errors));
  EXPECT_EQ(1, fs->Size());
  EXPECT_FALSE(errors.IsFatal());

  File *file = fs->Get(0);
  EXPECT_EQ(1, file->Size());
  EXPECT_EQ('a', file->At(0));
  EXPECT_EQ("base/testdata", file->Dirname());
  EXPECT_EQ("a.txt", file->Basename());
}

TEST_F(FileSetTest, NonExistentDiskEntry) {
  ASSERT_FALSE(FileSet::Builder().AddDiskFile("notafolder/notafile.txt").Build(&fs, &errors));
  EXPECT_EQ(nullptr, fs);
  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(
      "DiskFileError{errval_:2,path_:notafolder/notafile.txt,}\n",
      testing::PrintToString(errors));
}

TEST_F(FileSetTest, MultiEntry) {
  ASSERT_TRUE(FileSet::Builder()
                  .AddStringFile("base/testdata/b.txt", "b")
                  .AddDiskFile("base/testdata/a.txt")
                  .Build(&fs, &errors));
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
}

}  // namespace base
