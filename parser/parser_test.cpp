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

TEST_F(ParserTest, QualifiedNameSingleIdent) {
  MakeParser("foo");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(name));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(name.Errors().IsFatal());
  EXPECT_EQ("foo", Str(name.Get()));
}

TEST_F(ParserTest, QualifiedNameMultiIdent) {
  MakeParser("foo.bar.baz");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(name));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(name.Errors().IsFatal());
  EXPECT_EQ("foo.bar.baz", Str(name.Get()));
}

TEST_F(ParserTest, QualifiedNameTrailingDot) {
  MakeParser("foo.bar.baz.");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(name));
  EXPECT_TRUE(name.Errors().IsFatal());
  EXPECT_EQ("UnexpectedEOFError(0:11)\n", testing::PrintToString(name.Errors()));
}

TEST_F(ParserTest, SingleTypePrimitive) {
  MakeParser("int");

  Result<Type> type;
  Parser after = parser_->ParseSingleType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("K_INT", Str(type.Get()));
}

TEST_F(ParserTest, SingleTypeReference) {
  MakeParser("String");

  Result<Type> type;
  Parser after = parser_->ParseSingleType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("String", Str(type.Get()));
}

TEST_F(ParserTest, SingleTypeMultiReference) {
  MakeParser("java.lang.String");

  Result<Type> type;
  Parser after = parser_->ParseSingleType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("java.lang.String", Str(type.Get()));
}

TEST_F(ParserTest, SingleTypeBothFail) {
  MakeParser(";");

  Result<Type> type;
  Parser after = parser_->ParseSingleType(&type);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(type));
  EXPECT_TRUE(type.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:0)\n", testing::PrintToString(type.Errors()));
}

TEST_F(ParserTest, TypeNonArray) {
  MakeParser("int");

  Result<Type> type;
  Parser after = parser_->ParseType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("K_INT", Str(type.Get()));
}

TEST_F(ParserTest, TypeFail) {
  MakeParser(";");

  Result<Type> type;
  Parser after = parser_->ParseType(&type);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(type));
  EXPECT_TRUE(type.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:0)\n", testing::PrintToString(type.Errors()));
}

TEST_F(ParserTest, TypeArray) {
  MakeParser("int[]");

  Result<Type> type;
  Parser after = parser_->ParseType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("array<K_INT>", Str(type.Get()));
}

TEST_F(ParserTest, TypeArrayFail) {
  MakeParser("int[;");

  Result<Type> type;
  Parser after = parser_->ParseType(&type);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(type));
  EXPECT_TRUE(type.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:4)\n", testing::PrintToString(type.Errors()));
}

} // namespace parser
