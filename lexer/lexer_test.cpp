#include "lexer/lexer.h"
#include "lexer/lexer_internal.h"
#include "third_party/gtest/gtest.h"

using base::FileSet;

namespace lexer {

class LexerTest : public ::testing::Test {
 protected:
  void SetUp() { fs = nullptr; }

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
  ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.joos", "").Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  EXPECT_EQ(0u, tokens[0].size());
}

// Tests that the SymbolLiterals are sorted by length, so that we do maximal
// munch correctly.
TEST_F(LexerTest, SymbolLiterals) {
  for (int i = 1; i < lexer::internal::kNumSymbolLiterals; ++i) {
    EXPECT_LE(lexer::internal::kSymbolLiterals[i].first.size(),
              lexer::internal::kSymbolLiterals[i - 1].first.size());
  }
}

TEST_F(LexerTest, Symbols) {
  FileSet* fs = nullptr;
  ASSERT_TRUE(FileSet::Builder()
                  .AddStringFile("foo.joos", "<<=>>====!=!&&&|||+-*/%(){}[];,.")
                  .Build(&fs));
  unique_ptr<FileSet> deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(26u, tokens[0].size());

  EXPECT_EQ(LT, tokens[0][0].type);
  EXPECT_EQ(LE, tokens[0][1].type);
  EXPECT_EQ(GT, tokens[0][2].type);
  EXPECT_EQ(GE, tokens[0][3].type);
  EXPECT_EQ(EQ, tokens[0][4].type);
  EXPECT_EQ(ASSG, tokens[0][5].type);
  EXPECT_EQ(NEQ, tokens[0][6].type);
  EXPECT_EQ(NOT, tokens[0][7].type);
  EXPECT_EQ(AND, tokens[0][8].type);
  EXPECT_EQ(BAND, tokens[0][9].type);
  EXPECT_EQ(OR, tokens[0][10].type);
  EXPECT_EQ(BOR, tokens[0][11].type);
  EXPECT_EQ(ADD, tokens[0][12].type);
  EXPECT_EQ(SUB, tokens[0][13].type);
  EXPECT_EQ(MUL, tokens[0][14].type);
  EXPECT_EQ(DIV, tokens[0][15].type);
  EXPECT_EQ(MOD, tokens[0][16].type);
  EXPECT_EQ(LPAREN, tokens[0][17].type);
  EXPECT_EQ(RPAREN, tokens[0][18].type);
  EXPECT_EQ(LBRACE, tokens[0][19].type);
  EXPECT_EQ(RBRACE, tokens[0][20].type);
  EXPECT_EQ(LBRACK, tokens[0][21].type);
  EXPECT_EQ(RBRACK, tokens[0][22].type);
  EXPECT_EQ(SEMI, tokens[0][23].type);
  EXPECT_EQ(COMMA, tokens[0][24].type);
  EXPECT_EQ(DOT, tokens[0][25].type);
}

TEST_F(LexerTest, Whitespace) {
  ASSERT_TRUE(
      FileSet::Builder().AddStringFile("foo.joos", " \n    \r   \t").Build(
          &fs));

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(1u, tokens[0].size());

  EXPECT_EQ(WHITESPACE, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 11), tokens[0][0].pos);
}

TEST_F(LexerTest, Comment) {
  ASSERT_TRUE(
      FileSet::Builder().AddStringFile("foo.joos", "// foo bar\n/*baz*/").Build(
          &fs));

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(2u, tokens[0].size());

  EXPECT_EQ(LINE_COMMENT, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 11), tokens[0][0].pos);

  EXPECT_EQ(BLOCK_COMMENT, tokens[0][1].type);
  EXPECT_EQ(PosRange(0, 11, 18), tokens[0][1].pos);
}

TEST_F(LexerTest, LineCommentAtEof) {
  ASSERT_TRUE(
      FileSet::Builder().AddStringFile("foo.joos", "// foo bar").Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);

  EXPECT_EQ(1u, tokens[0].size());

  EXPECT_EQ(LINE_COMMENT, tokens[0][0].type);
  EXPECT_EQ(PosRange(0, 0, 10), tokens[0][0].pos);
}

TEST_F(LexerTest, SimpleInteger) {
  ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.joos", "123").Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, LeadingZeroInteger) {
  ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.joos", "023").Build(&fs));

  ASSERT_ANY_THROW({ LexJoosFiles(fs, &tokens, &errors); });
}

TEST_F(LexerTest, OnlyZero) {
  ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.joos", "0").Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
}

TEST_F(LexerTest, SimpleIdentifier) {
  ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.joos", "foo").Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, NumberBeforeIdentifier) {
  ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.joos", "3m").Build(&fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(2u, tokens[0].size());

  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 1, 2)), tokens[0][1]);
}

TEST_F(LexerTest, CommentBetweenIdentifiers) {
  ASSERT_TRUE(
      FileSet::Builder().AddStringFile("foo.joos", "abc/*foobar*/def").Build(
          &fs));

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(3u, tokens[0].size());

  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
  EXPECT_EQ(Token(BLOCK_COMMENT, PosRange(0, 3, 13)), tokens[0][1]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 13, 16)), tokens[0][2]);
}

// TODO: unclosed block comment test.

}  // namespace base
