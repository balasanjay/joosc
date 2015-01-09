#include "lexer/lexer.h"
#include "third_party/gtest/gtest.h"

using base::FileSet;

namespace lexer {


class LexerTest : public ::testing::Test {
protected:
  vector<vector<Token>> tokens;
  vector<Error> errors;
};

TEST_F(LexerTest, EmptyFile) {
  FileSet* fs = nullptr;
  ASSERT_TRUE(FileSet::Builder()
    .AddStringFile("foo.joos", "")
    .Build(&fs));
  unique_ptr<FileSet> deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);
  EXPECT_EQ(0u, tokens[0].size());
}

TEST_F(LexerTest, Whitespace) {
  FileSet* fs = nullptr;
  ASSERT_TRUE(FileSet::Builder()
    .AddStringFile("foo.joos", " \n    \r   \t")
    .Build(&fs));
  unique_ptr<FileSet> deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(1u, tokens[0].size());

  EXPECT_EQ(WHITESPACE, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 11), tokens[0][0].pos);
}

TEST_F(LexerTest, Comment) {
  FileSet* fs = nullptr;
  ASSERT_TRUE(FileSet::Builder()
    .AddStringFile("foo.joos", "// foo bar\n/*baz*/")
    .Build(&fs));
  unique_ptr<FileSet> deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(2u, tokens[0].size());

  EXPECT_EQ(LINE_COMMENT, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 11), tokens[0][0].pos);

  EXPECT_EQ(BLOCK_COMMENT, tokens[0][1].type);
  EXPECT_EQ(PosRange(0, 11, 18), tokens[0][1].pos);
}

TEST_F(LexerTest, LineCommentAtEof) {
  FileSet* fs = nullptr;
  ASSERT_TRUE(FileSet::Builder()
    .AddStringFile("foo.joos", "// foo bar")
    .Build(&fs));
  unique_ptr<FileSet> deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(1u, tokens[0].size());

  EXPECT_EQ(LINE_COMMENT, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 10), tokens[0][0].pos);
}

// TODO: unclosed block comment test.

}
