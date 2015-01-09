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

  void LexString(string s) {
    ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.joos", s).Build(&fs));
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

// Tests that the SymbolLiterals are sorted by length, so that we do maximal
// munch correctly.
TEST_F(LexerTest, SymbolLiterals) {
  for (int i = 1; i < lexer::internal::kNumSymbolLiterals; ++i) {
    EXPECT_LE(lexer::internal::kSymbolLiterals[i].first.size(),
              lexer::internal::kSymbolLiterals[i - 1].first.size());
  }
}

TEST_F(LexerTest, Symbols) {
  ASSERT_TRUE(FileSet::Builder()
                  .AddStringFile("foo.joos", "<<=>>====!=!&&&|||+-*/%(){}[];,.")
                  .Build(&fs));

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

TEST_F(LexerTest, UnclosedBlockComment) {
  ASSERT_ANY_THROW({ LexString("hello /* there \n\n end"); });
}

TEST_F(LexerTest, SimpleInteger) {
  LexString("123");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, LeadingZeroInteger) {
  ASSERT_ANY_THROW({ LexString("023"); });
}

TEST_F(LexerTest, OnlyZero) {
  LexString("0");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
}

TEST_F(LexerTest, SimpleIdentifier) {
  LexString("foo");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, NumberBeforeIdentifier) {
  LexString("3m");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(2u, tokens[0].size());

  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 1, 2)), tokens[0][1]);
}

TEST_F(LexerTest, CommentBetweenIdentifiers) {
  LexString("abc/*foobar*/def");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(3u, tokens[0].size());

  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
  EXPECT_EQ(Token(BLOCK_COMMENT, PosRange(0, 3, 13)), tokens[0][1]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 13, 16)), tokens[0][2]);
}

TEST_F(LexerTest, OnlyString) {
  LexString("\"goober\"");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());

  EXPECT_EQ(Token(STRING, PosRange(0, 0, 8)), tokens[0][0]);
}

TEST_F(LexerTest, UnendedString) {
  ASSERT_ANY_THROW({ LexString("\"goober"); });
}

TEST_F(LexerTest, UnendedEscapedQuoteString) {
  ASSERT_ANY_THROW({ LexString("\"goober\\\""); });
}

TEST_F(LexerTest, StringOverNewline) {
  ASSERT_ANY_THROW({ LexString("\"foo\nbar\""); });
}

TEST_F(LexerTest, StringEscapedQuote) {
  LexString("\"foo\\\"bar\"");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());

  EXPECT_EQ(Token(STRING, PosRange(0, 0, 10)), tokens[0][0]);
}

TEST_F(LexerTest, AssignStringTest) {
  LexString("string foo = \"foo\";");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(8u, tokens[0].size());
}

TEST_F(LexerTest, SimpleChar) {
  LexString("'a'");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(CHAR, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, MultipleChars) {
  ASSERT_ANY_THROW({ LexString("'ab'"); });
}

TEST_F(LexerTest, EscapedChars) {
  LexString("'\\b''\\t''\\n''\\f''\\r''\\\'''\\\\'");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(7u, tokens[0].size());
  for (auto token : tokens[0]) {
    EXPECT_EQ(CHAR, token.type);
  }
}

TEST_F(LexerTest, EscapedOctalChars) {
  LexString("'\\0''\\1''\\123''\\001''\\377'");
  ASSERT_TRUE(errors.empty());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(5u, tokens[0].size());
  for (auto token : tokens[0]) {
    EXPECT_EQ(CHAR, token.type);
  }
}

TEST_F(LexerTest, BadEscapedChar) {
  ASSERT_ANY_THROW({ LexString("'\\a'"); });
  ASSERT_ANY_THROW({ LexString("'\\0a'"); });
  ASSERT_ANY_THROW({ LexString("'\\456'"); });
  ASSERT_ANY_THROW({ LexString("'\\378'"); });
  ASSERT_ANY_THROW({ LexString("'\\399'"); });
}

}  // namespace base
