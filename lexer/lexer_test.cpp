#include "lexer/lexer.h"
#include "third_party/gtest/gtest.h"

using base::FileSet;

namespace lexer {

class LexerTest : public ::testing::Test {
protected:
  void SetUp() {
    fs = nullptr;
  }

  void TearDown() {
    if (fs != nullptr) {
      delete fs;
      fs = nullptr;
    }
  }

  vector<vector<Token>> tokens;
  vector<Error> errors;
  FileSet* fs;
};

TEST_F(LexerTest, EmptyFile) {
  ASSERT_TRUE(FileSet::Builder()
    .AddStringFile("foo.joos", "")
    .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  EXPECT_EQ(0u, tokens[0].size());
}

TEST_F(LexerTest, Whitespace) {
  ASSERT_TRUE(FileSet::Builder()
    .AddStringFile("foo.joos", " \n    \r   \t")
    .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(1u, tokens[0].size());

  EXPECT_EQ(WHITESPACE, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 11), tokens[0][0].pos);
}

TEST_F(LexerTest, Comment) {
  ASSERT_TRUE(FileSet::Builder()
    .AddStringFile("foo.joos", "// foo bar\n/*baz*/")
    .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(2u, tokens[0].size());

  EXPECT_EQ(LINE_COMMENT, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 11), tokens[0][0].pos);

  EXPECT_EQ(BLOCK_COMMENT, tokens[0][1].type);
  EXPECT_EQ(PosRange(0, 11, 18), tokens[0][1].pos);
}

TEST_F(LexerTest, LineCommentAtEof) {
  ASSERT_TRUE(FileSet::Builder()
    .AddStringFile("foo.joos", "// foo bar")
    .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(1u, tokens[0].size());

  EXPECT_EQ(LINE_COMMENT, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 10), tokens[0][0].pos);
}

TEST_F(LexerTest, SimpleInteger) {
  ASSERT_TRUE(
      FileSet::Builder()
      .AddStringFile("foo.joos", "123")
      .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(1, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, LeadingZeroInteger) {
  ASSERT_TRUE(
      FileSet::Builder()
      .AddStringFile("foo.joos", "023")
      .Build(&fs));

  ASSERT_ANY_THROW({
    LexJoosFiles(fs, &tokens, &errors);
  });
}

TEST_F(LexerTest, OnlyZero) {
  ASSERT_TRUE(
      FileSet::Builder()
      .AddStringFile("foo.joos", "0")
      .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(1, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
}

TEST_F(LexerTest, SimpleIdentifier) {
  ASSERT_TRUE(
      FileSet::Builder()
      .AddStringFile("foo.joos", "foo")
      .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(1, tokens[0].size());
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, NumberBeforeIdentifier) {
  ASSERT_TRUE(
      FileSet::Builder()
      .AddStringFile("foo.joos", "3m")
      .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(2, tokens[0].size());

  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 1, 2)), tokens[0][1]);
}

TEST_F(LexerTest, CommentBetweenIdentifiers) {
  ASSERT_TRUE(
      FileSet::Builder()
      .AddStringFile("foo.joos", "abc/*foobar*/def")
      .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(3, tokens[0].size());

  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
  EXPECT_EQ(Token(BLOCK_COMMENT, PosRange(0, 3, 13)), tokens[0][1]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 13, 16)), tokens[0][2]);
}

// TODO: unclosed block comment test.

}  // namespace base

