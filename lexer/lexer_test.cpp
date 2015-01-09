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

  void LexString(string s) {
    ASSERT_TRUE(
        FileSet::Builder()
        .AddStringFile("foo.joos", s)
        .Build(&fs));
    LexJoosFiles(fs, &tokens, &errors);
  }

  vector<vector<Token>> tokens;
  vector<Error> errors;
  FileSet* fs;
};

TEST_F(LexerTest, EmptyFile) {
  LexString("");
  EXPECT_EQ(0u, tokens[0].size());
}

TEST_F(LexerTest, Whitespace) {
  LexString(" \n    \r   \t");

  EXPECT_EQ(1u, tokens[0].size());

  EXPECT_EQ(WHITESPACE, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 11), tokens[0][0].pos);
}

TEST_F(LexerTest, Comment) {
  LexString("// foo bar\n/*baz*/");

  EXPECT_EQ(2u, tokens[0].size());

  EXPECT_EQ(LINE_COMMENT, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 11), tokens[0][0].pos);

  EXPECT_EQ(BLOCK_COMMENT, tokens[0][1].type);
  EXPECT_EQ(PosRange(0, 11, 18), tokens[0][1].pos);
}

TEST_F(LexerTest, LineCommentAtEof) {
  LexString("// foo bar");

  EXPECT_EQ(1u, tokens[0].size());

  EXPECT_EQ(LINE_COMMENT, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 10), tokens[0][0].pos);
}

TEST_F(LexerTest, SimpleInteger) {
  LexString("123");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(1, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, LeadingZeroInteger) {
  ASSERT_ANY_THROW({
    LexString("023");
  });
}

TEST_F(LexerTest, OnlyZero) {
  LexString("0");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(1, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
}

TEST_F(LexerTest, SimpleIdentifier) {
  LexString("foo");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(1, tokens[0].size());
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, NumberBeforeIdentifier) {
  LexString("3m");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(2, tokens[0].size());

  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 1, 2)), tokens[0][1]);
}

TEST_F(LexerTest, CommentBetweenIdentifiers) {
  LexString("abc/*foobar*/def");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(3, tokens[0].size());

  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
  EXPECT_EQ(Token(BLOCK_COMMENT, PosRange(0, 3, 13)), tokens[0][1]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 13, 16)), tokens[0][2]);
}

TEST_F(LexerTest, OnlyString) {
  LexString("\"goober\"");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(1, tokens[0].size());

  EXPECT_EQ(Token(STRING, PosRange(0, 0, 8)), tokens[0][0]);
}

TEST_F(LexerTest, UnendedString) {
  ASSERT_ANY_THROW({
    LexString("\"goober");
  });
}

TEST_F(LexerTest, UnendedEscapedQuoteString) {
  ASSERT_ANY_THROW({
    LexString("\"goober\\\"");
  });
}

TEST_F(LexerTest, StringOverNewline) {
  ASSERT_ANY_THROW({
    LexString("\"foo\nbar\"");
  });
}

TEST_F(LexerTest, StringEscapedQuote) {
  LexString("\"foo\\\"bar\"");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(1, tokens[0].size());

  EXPECT_EQ(Token(STRING, PosRange(0, 0, 10)), tokens[0][0]);
}

TEST_F(LexerTest, AssignStringTest) {
  ASSERT_TRUE(
      FileSet::Builder()
      .AddStringFile("foo.joos", "string foo = \"foo\";")
      .Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1, tokens.size());
  ASSERT_EQ(8, tokens[0].size());
}

// TODO: unclosed block comment test.

}  // namespace base

