#include "lexer/lexer.h"
#include "parser/parser_internal.h"
#include "third_party/gtest/gtest.h"

using base::ErrorList;
using base::FileSet;
using base::PosRange;
using lexer::Token;
using parser::internal::Result;
using lexer::DOT;

// Help out gtest's macros with boolean conversions.
#define b(buh) true && buh

namespace parser {

class ParserTest : public ::testing::Test {
 protected:
  void SetUp() {
    fs_.reset();
    parser_.reset();
  }

  void TearDown() {
    fs_.reset();
    parser_.reset();
  }

  void MakeParser(string s) {
    ErrorList errors;

    // Create file set.
    FileSet* fs;
    ASSERT_TRUE(
        FileSet::Builder().AddStringFile("foo.joos", s).Build(&fs, &errors));
    fs_.reset(fs);

    // Lex tokens.
    lexer::LexJoosFiles(fs, &tokens, &errors);

    // Make sure it worked.
    ASSERT_EQ(1u, tokens.size());
    ASSERT_FALSE(errors.IsFatal());

    parser_.reset(new Parser(fs, fs->Get(0), &tokens[0]));
  }

  vector<vector<Token>> tokens;
  unique_ptr<FileSet> fs_;
  unique_ptr<Parser> parser_;
};

template <typename T>
string Str(const T& t) {
  std::stringstream s;
  t.PrintTo(&s);
  return s.str();
}

template <typename T>
string Str(T* t) {
  std::stringstream s;
  t->PrintTo(&s);
  return s.str();
}

TEST_F(ParserTest, QualifiedNameNoLeadingIdent) {
  MakeParser(";");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(name));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n", testing::PrintToString(name.Errors()));
}

TEST_F(ParserTest, SingleIdent) {
  MakeParser("foo");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(name));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(name.Errors().IsFatal());
  EXPECT_EQ("foo", Str(name.Get()));
}

TEST_F(ParserTest, MultiIdent) {
  MakeParser("foo.bar.baz");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(name));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(name.Errors().IsFatal());
  EXPECT_EQ("foo.bar.baz", Str(name.Get()));
}

TEST_F(ParserTest, TrailingDot) {
  MakeParser("foo.bar.baz.");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(name));
  EXPECT_FALSE(after.IsAtEnd());
  EXPECT_EQ(DOT, after.GetNext().type);
  EXPECT_FALSE(name.Errors().IsFatal());
  EXPECT_EQ("foo.bar.baz", Str(name.Get()));
}

} // namespace parser