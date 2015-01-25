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
    lexer::StripSkippableTokens(&tokens[0]);

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

TEST_F(ParserTest, ArgumentListNone) {
  MakeParser(")"); // TODO: Crashes on empty string here.
  Result<ArgumentList> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ("", Str(args.Get()));
}

TEST_F(ParserTest, ArgumentListOne) {
  MakeParser("foo.bar");
  Result<ArgumentList> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ("foo.bar", Str(args.Get()));
}

TEST_F(ParserTest, ArgumentListMany) {
  MakeParser("a,b, c, d  , e");
  Result<ArgumentList> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ("a, b, c, d, e", Str(args.Get()));
}

TEST_F(ParserTest, ArgumentListHangingComma) {
  MakeParser("a, b,)");
  Result<ArgumentList> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(args));
}

TEST_F(ParserTest, ArgumentListStartingComma) {
  MakeParser(", a, b, c)");
  Result<ArgumentList> args;
  Parser after = parser_->ParseArgumentList(&args);
  // Shouldn't parse anything, since arg list is optional.
  // TODO: Do we actually want this to fail if it doesn't stop at at an RPAREN?
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ("", Str(args.Get()));
}

TEST_F(ParserTest, PrimaryNewSuccess) {
  MakeParser("new int[]");

  Result<Expr> newExpr;
  Parser after = parser_->ParsePrimary(&newExpr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(newExpr));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(newExpr.Errors().IsFatal());
  EXPECT_EQ("new<array<K_INT>>()", Str(newExpr.Get()));
}

TEST_F(ParserTest, PrimaryNewFail) {
  MakeParser("new ;");

  Result<Expr> newExpr;
  Parser after = parser_->ParsePrimary(&newExpr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(newExpr));
  EXPECT_TRUE(newExpr.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:4)\n", testing::PrintToString(newExpr.Errors()));
}

TEST_F(ParserTest, PrimaryCallBaseFail) {
  MakeParser(";");

  Result<Expr> expr;
  Parser after = parser_->ParsePrimary(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_TRUE(expr.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:0)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, PrimaryCallBaseNoEnd) {
  MakeParser("3");

  Result<Expr> expr;
  Parser after = parser_->ParsePrimary(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_FALSE(expr.Errors().IsFatal());
  EXPECT_EQ("INTEGER", Str(expr.Get()));
}

TEST_F(ParserTest, PrimaryCallBaseWithEnd) {
  MakeParser("3()");

  Result<Expr> expr;
  Parser after = parser_->ParsePrimary(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_FALSE(expr.Errors().IsFatal());
  EXPECT_EQ("INTEGER()", Str(expr.Get()));
}

TEST_F(ParserTest, DISABLED_PrimaryCallBaseWithEndFail) {
  MakeParser("3(]");

  Result<Expr> expr;
  Parser after = parser_->ParsePrimary(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_TRUE(expr.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:2)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, NewExprNoType) {
  MakeParser("new if");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_TRUE(expr.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:4)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, NewExprAfterTypeEOF) {
  MakeParser("new int");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_TRUE(expr.Errors().IsFatal());
  EXPECT_EQ("UnexpectedEOFError(0:6)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, NewExprAfterTypeNoParenOrBrack) {
  MakeParser("new int;");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_TRUE(expr.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:7)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, NewExprInvalidArgList) {
  MakeParser("new int(;)");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_TRUE(expr.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:8)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, NewExprClassWithPrimaryEnd) {
  MakeParser("new int().f");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_FALSE(expr.Errors().IsFatal());
  EXPECT_EQ("new<K_INT>().f", Str(expr.Get()));
}

TEST_F(ParserTest, NewExprClassWithNoPrimaryEnd) {
  MakeParser("new int();");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_FALSE(expr.Errors().IsFatal());
  EXPECT_EQ("new<K_INT>()", Str(expr.Get()));
}

TEST_F(ParserTest, NewExprArrayNoSizeNoEnd) {
  MakeParser("new int[]");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_FALSE(expr.Errors().IsFatal());
  EXPECT_EQ("new<array<K_INT>>()", Str(expr.Get()));
}

TEST_F(ParserTest, NewExprArraySizeError) {
  MakeParser("new int[;]");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_TRUE(expr.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:8)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, NewExprArrayEndSuccess) {
  MakeParser("new int[3].f");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_FALSE(expr.Errors().IsFatal());
  EXPECT_EQ("new<array<K_INT>>(INTEGER).f", Str(expr.Get()));
}

TEST_F(ParserTest, NewExprArrayEndFail) {
  MakeParser("new int[3];");

  Result<Expr> expr;
  Parser after = parser_->ParseNewExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_FALSE(expr.Errors().IsFatal());
  EXPECT_EQ("new<array<K_INT>>(INTEGER)", Str(expr.Get()));
}

} // namespace parser
