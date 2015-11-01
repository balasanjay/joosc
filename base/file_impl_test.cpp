#include "base/file_impl.h"

#include "base/error.h"
#include "base/file.h"
#include "gtest/gtest.h"

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

TEST(StringFileTest, LineMap) {
  StringFile sf("foo.txt", "abc\ncde\nfgh\n\nijk");

  int line = -1;
  int col = -1;

  // Query before any newlines.
  {
    sf.IndexToLineCol(0, &line, &col);
    EXPECT_EQ(0, line);
    EXPECT_EQ(0, col);
  }

  // Query first newline.
  {
    sf.IndexToLineCol(3, &line, &col);
    EXPECT_EQ(0, line);
    EXPECT_EQ(3, col);
  }

  // Query first character in second line.
  {
    sf.IndexToLineCol(4, &line, &col);
    EXPECT_EQ(1, line);
    EXPECT_EQ(0, col);
  }

  // Query newline in second line.
  {
    sf.IndexToLineCol(7, &line, &col);
    EXPECT_EQ(1, line);
    EXPECT_EQ(3, col);
  }

  // Query value after all newlines.
  {
    sf.IndexToLineCol(14, &line, &col);
    EXPECT_EQ(4, line);
    EXPECT_EQ(1, col);
  }
}

TEST(StringFileTest, LineMapEmptyFile) {
  // Just testing that creating an empty-file doesn't crash.
  StringFile sf("foo.txt", "");
}

TEST(StringFileTest, PrintRangePtr) {
  const string file =
      "class Foo {\n"
      "  String bar = 3; /* foo bar \n"
      "}\n";

  const string expected =
      "  String bar = 3; /* foo bar \n"
      "                  ^~         ";

  FileSet* fs;
  ErrorList errors;
  ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.txt", file).Build(&fs, &errors));
  uptr<FileSet> fs_deleter(fs);

  std::stringstream ss;
  PrintRangePtr(&ss, OutputOptions::kSimpleOutput, fs, PosRange(0, 30, 32));

  EXPECT_EQ(expected, ss.str());
}

TEST(DiskFileTest, Simple) {
  File* df = nullptr;
  ErrorList errors;
  ASSERT_TRUE(DiskFile::LoadFile("base/testdata/testfile.txt", &df, &errors));
  EXPECT_EQ(11, df->Size());
  ValidateFile(*df, 0, "test file\n");
  delete (DiskFile*)df;
}

}  // namespace base
