#include "lexer/lexer.h"
#include "lexer/lexer_internal.h"
#include "third_party/gtest/gtest.h"

using base::ErrorList;
using base::FileSet;
using base::PosRange;

namespace lexer {

class LexerTest : public ::testing::Test {
 protected:
  void SetUp() { fs = nullptr; }

  void TearDown() {
    delete fs;
    fs = nullptr;
  }

  void LexString(string s) {
    ASSERT_TRUE(FileSet::Builder().AddStringFile("foo.joos", s).Build(&fs, &errors));
    LexJoosFiles(fs, &tokens, &errors);
  }

  vector<vector<Token>> tokens;
  ErrorList errors;
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
  LexString("<<=>>====!=!&&&|||+-*/%(){}[];,.");

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
  LexString("hello /* there \n\n end");
  EXPECT_EQ(1, errors.Size());
  EXPECT_EQ("UnclosedBlockCommentError(0:6-8)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, SimpleInteger) {
  LexString("123");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, LeadingZeroInteger) {
  LexString("023");

  EXPECT_EQ(1, errors.Size());
  EXPECT_EQ("LeadingZeroInIntLitError(0:0)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, OnlyZero) {
  LexString("0");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
}

TEST_F(LexerTest, SimpleIdentifier) {
  LexString("foo");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, NumberBeforeIdentifier) {
  LexString("3m");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(2u, tokens[0].size());

  EXPECT_EQ(Token(INTEGER, PosRange(0, 0, 1)), tokens[0][0]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 1, 2)), tokens[0][1]);
}

TEST_F(LexerTest, NumberIdentifier) {
  LexString("foo123bar890");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 12)), tokens[0][0]);
}

TEST_F(LexerTest, UnderscoreIdentifier) {
  LexString("MAX_VALUE");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 9)), tokens[0][0]);
}

TEST_F(LexerTest, DollarIdentifier) {
  LexString("cash$");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 5)), tokens[0][0]);
}

TEST_F(LexerTest, CommentBetweenIdentifiers) {
  LexString("abc/*foobar*/def");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(3u, tokens[0].size());

  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 0, 3)), tokens[0][0]);
  EXPECT_EQ(Token(BLOCK_COMMENT, PosRange(0, 3, 13)), tokens[0][1]);
  EXPECT_EQ(Token(IDENTIFIER, PosRange(0, 13, 16)), tokens[0][2]);
}

TEST_F(LexerTest, Keywords) {
  LexString("while true null char if const");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(11u, tokens[0].size());
  EXPECT_EQ(K_WHILE, tokens[0][0].type);
  EXPECT_EQ(K_TRUE, tokens[0][2].type);
  EXPECT_EQ(K_NULL, tokens[0][4].type);
  EXPECT_EQ(K_CHAR, tokens[0][6].type);
  EXPECT_EQ(K_IF, tokens[0][8].type);
  EXPECT_EQ(K_CONST, tokens[0][10].type);
}

TEST_F(LexerTest, KeywordPrefix) {
  LexString("whil e");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(3u, tokens[0].size());
  EXPECT_EQ(IDENTIFIER, tokens[0][0].type);
  EXPECT_EQ(IDENTIFIER, tokens[0][2].type);
}

TEST_F(LexerTest, KeywordPrefixAtEOF) {
  LexString("whil");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(IDENTIFIER, tokens[0][0].type);
}

TEST_F(LexerTest, OnlyString) {
  LexString("\"goober\"");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());

  EXPECT_EQ(Token(STRING, PosRange(0, 0, 8)), tokens[0][0]);
}

TEST_F(LexerTest, UnendedString) {
  LexString("\"goober");
  EXPECT_EQ(1, errors.Size());
  EXPECT_EQ("UnclosedStringLitError(0:0)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, UnendedStringAtEOF) {
  LexString("\"");
  EXPECT_EQ(1, errors.Size());
  EXPECT_EQ("UnclosedStringLitError(0:0)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, UnendedEscapedQuoteString) {
  LexString("foo\"goober\\\"");
  EXPECT_EQ(1, errors.Size());
  EXPECT_EQ("UnclosedStringLitError(0:3)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, StringOverNewline) {
  LexString("baz\"foo\nbar\"");
  EXPECT_EQ(1, errors.Size());
  EXPECT_EQ("UnclosedStringLitError(0:3)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, StringWithEscapedOctal) {
  LexString("\"Hello Mr. \\333. How are you doing this fine \\013?\"");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  EXPECT_EQ(Token(STRING, PosRange(0, 0, 51)), tokens[0][0]);
}

TEST_F(LexerTest, StringWithOutOfRangeOctalWorks) {
  // Lexes a '\40' and then a 0. Works in java.
  LexString("\"What the heck is a \\400?\"");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  EXPECT_EQ(Token(STRING, PosRange(0, 0, 26)), tokens[0][0]);
}

TEST_F(LexerTest, StringWithBadEscape) {
  LexString("\"Lol: \\91\"");
  EXPECT_EQ("InvalidCharacterEscapeError(0:6)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, StringEscapedQuote) {
  LexString("\"foo\\\"bar\"");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());

  EXPECT_EQ(Token(STRING, PosRange(0, 0, 10)), tokens[0][0]);
}

TEST_F(LexerTest, AssignStringTest) {
  LexString("string foo = \"foo\";");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(8u, tokens[0].size());
}

TEST_F(LexerTest, SimpleChar) {
  LexString("'a'");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(1u, tokens[0].size());
  EXPECT_EQ(Token(CHAR, PosRange(0, 0, 3)), tokens[0][0]);
}

TEST_F(LexerTest, MultipleChars) {
  LexString("'ab'");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0-2)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, EscapedChars) {
  LexString("'\\b''\\t''\\n''\\f''\\r''\\\'''\\\\'");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(7u, tokens[0].size());
  for (auto token : tokens[0]) {
    EXPECT_EQ(CHAR, token.type);
  }
}

TEST_F(LexerTest, EscapedOctalChars) {
  LexString("'\\0''\\1''\\123''\\001''\\377'");
  ASSERT_FALSE(errors.IsFatal());
  ASSERT_EQ(1u, tokens.size());
  ASSERT_EQ(5u, tokens[0].size());
  for (auto token : tokens[0]) {
    EXPECT_EQ(CHAR, token.type);
  }
}

TEST_F(LexerTest, BadEscapedChar) {
  LexString("'\\a'");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterEscapeError(0:0-2)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, BadTooHighEscapedChar) {
  LexString("'\\456'");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0-4)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, UnexpectedChar) {
  LexString("\\");
  EXPECT_EQ(1, errors.Size());
  EXPECT_EQ("UnexpectedCharError(0:0)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, BadBarelyTooHighEscapedChar) {
  LexString("'\\378'");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0-4)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, MultipleCharsWithEscape) {
  LexString("'\\0a'");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0-3)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, EmptyChar) {
  LexString("''");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, ThreeApostropheChar) {
  LexString("'''");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, UnclosedCharAtEOF) {
  LexString("'");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, UnclosedChar2AtEOF) {
  LexString("'a");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0-2)", testing::PrintToString(*errors.Get(0)));
}

TEST_F(LexerTest, UnclosedChar) {
  LexString("'foobar");
  ASSERT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCharacterLitError(0:0-2)", testing::PrintToString(*errors.Get(0)));
}

}  // namespace base
