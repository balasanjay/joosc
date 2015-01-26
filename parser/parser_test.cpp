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

class TestExpr : public Expr {
public :
  void PrintTo(std::ostream* os) const override {
    *os << "TEST";
  }
};

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
  MakeParser(")");
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

TEST_F(ParserTest, ArgumentListNestedExpr) {
  MakeParser("a, (1 + b))");
  Result<ArgumentList> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ("a, (INTEGER ADD b)", Str(args.Get()));
}

TEST_F(ParserTest, ArgumentListBadExpr) {
  MakeParser("a, ;)");
  Result<ArgumentList> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(args));
  EXPECT_EQ("UnexpectedTokenError(0:3)\n", testing::PrintToString(args.Errors()));
}

TEST_F(ParserTest, ArgumentListStartingComma) {
  MakeParser(", a, b, c)");
  Result<ArgumentList> args;
  Parser after = parser_->ParseArgumentList(&args);
  // Shouldn't parse anything, since arg list is optional.
  // TODO: Do we actually want this to fail if it doesn't stop at at an
  // RPAREN?
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ("", Str(args.Get()));
}

TEST_F(ParserTest, DISABLED_PrimaryBaseShortCircuit) {
  MakeParser("");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedEOFError(0:0)\n", testing::PrintToString(primary.Errors()));
}

TEST_F(ParserTest, PrimaryBaseLit) {
  MakeParser("3");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primary));
  EXPECT_EQ("INTEGER", Str(primary.Get()));
}

TEST_F(ParserTest, PrimaryBaseThis) {
  MakeParser("this");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primary));
  EXPECT_EQ("this", Str(primary.Get()));
}

TEST_F(ParserTest, PrimaryBaseParens) {
  MakeParser("(3)");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primary));
  EXPECT_EQ("INTEGER", Str(primary.Get()));
}

TEST_F(ParserTest, PrimaryBaseParensExprFail) {
  MakeParser("(;)");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n", testing::PrintToString(primary.Errors()));
}

TEST_F(ParserTest, PrimaryBaseParensNoClosing) {
  MakeParser("(3;");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedTokenError(0:2)\n", testing::PrintToString(primary.Errors()));
}

TEST_F(ParserTest, PrimaryBaseQualifiedName) {
  MakeParser("a.b");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primary));
  EXPECT_EQ("a.b", Str(primary.Get()));
}

TEST_F(ParserTest, PrimaryBaseQualifiedNameFail) {
  MakeParser("a.b.;");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedTokenError(0:4)\n", testing::PrintToString(primary.Errors()));
}

TEST_F(ParserTest, PrimaryBaseAbort) {
  MakeParser(";");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n", testing::PrintToString(primary.Errors()));
}

TEST_F(ParserTest, PrimaryEndFailedArrayAccess) {
  MakeParser("[;]");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary.get(), &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n", testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndArrayAccessWithField) {
  MakeParser("[3].f");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary.release(), &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("TEST[INTEGER].f", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndArrayAccessNoTrailing) {
  MakeParser("[3]+5");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary.release(), &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("TEST[INTEGER]", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndNoAccess) {
  MakeParser(".f");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary.release(), &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("TEST.f", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, DISABLED_PrimaryEndNoArrayShortCircuit) {
  MakeParser("");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEndNoArrayAccess(primary.get(), &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedEOFError(0:0)\n", testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndNoArrayUnexpectedToken) {
  MakeParser(";");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEndNoArrayAccess(primary.get(), &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n", testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndNoArrayFieldFail) {
  MakeParser(".;");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEndNoArrayAccess(primary.get(), &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n", testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndNoArrayFieldWithEnd) {
  MakeParser(".f[0]");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEndNoArrayAccess(primary.release(), &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("TEST.f[INTEGER]", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndNoArrayFieldWithEndFail) {
  MakeParser(".f;");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEndNoArrayAccess(primary.release(), &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("TEST.f", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndNoArrayMethodFail) {
  MakeParser("(;)");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEndNoArrayAccess(primary.get(), &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n", testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndNoArrayMethodWithEnd) {
  MakeParser("().f");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEndNoArrayAccess(primary.release(), &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("TEST().f", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndNoArrayMethodWithEndFail) {
  MakeParser("();");
  unique_ptr<Expr> primary(new TestExpr());
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEndNoArrayAccess(primary.release(), &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("TEST()", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, DISABLED_UnaryEmptyShortCircuit) {
  MakeParser("");

  Result<Expr> unary;
  Parser after = parser_->ParseUnaryExpression(&unary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unary));
  EXPECT_EQ("UnexpectedEOFError(0:0)\n", testing::PrintToString(unary.Errors()));
}

TEST_F(ParserTest, UnaryIsUnary) {
  MakeParser("-3");

  Result<Expr> unary;
  Parser after = parser_->ParseUnaryExpression(&unary);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unary));
  EXPECT_EQ("(SUB INTEGER)", Str(unary.Get()));
}

TEST_F(ParserTest, UnaryOpFail) {
  MakeParser("-;");

  Result<Expr> unary;
  Parser after = parser_->ParseUnaryExpression(&unary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unary));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n", testing::PrintToString(unary.Errors()));
}

TEST_F(ParserTest, UnaryIsCast) {
  MakeParser("(int) 3");

  Result<Expr> unary;
  Parser after = parser_->ParseUnaryExpression(&unary);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unary));
  EXPECT_EQ("cast<K_INT>(INTEGER)", Str(unary.Get()));
}

TEST_F(ParserTest, UnaryCastFailIsPrimary) {
  MakeParser("3");

  Result<Expr> unary;
  Parser after = parser_->ParseUnaryExpression(&unary);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unary));
  EXPECT_EQ("INTEGER", Str(unary.Get()));
}

TEST_F(ParserTest, CastSuccess) {
  MakeParser("(int) 3");

  Result<Expr> cast;
  Parser after = parser_->ParseCastExpression(&cast);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(cast));
  EXPECT_EQ("cast<K_INT>(INTEGER)", Str(cast.Get()));
}

TEST_F(ParserTest, CastTypeFail) {
  MakeParser("(;) 3");

  Result<Expr> cast;
  Parser after = parser_->ParseCastExpression(&cast);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(cast));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n", testing::PrintToString(cast.Errors()));
}

TEST_F(ParserTest, CastExprFail) {
  MakeParser("(int) ;");

  Result<Expr> cast;
  Parser after = parser_->ParseCastExpression(&cast);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(cast));
  EXPECT_EQ("UnexpectedTokenError(0:6)\n", testing::PrintToString(cast.Errors()));
}

TEST_F(ParserTest, ExprUnaryFail) {
  MakeParser(";");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, ExprOnlyUnary) {
  MakeParser("3");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_EQ("INTEGER", Str(expr.Get()));
}

TEST_F(ParserTest, ExprUnaryBinFail) {
  MakeParser("-3+;");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_EQ("UnexpectedTokenError(0:3)\n", testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, ExprLeftAssoc) {
  MakeParser("a+b+c");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_EQ("((a ADD b) ADD c)", Str(expr.Get()));
}

TEST_F(ParserTest, ExprRightAssoc) {
  MakeParser("a = b = c");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_EQ("(a ASSG (b ASSG c))", Str(expr.Get()));
}

TEST_F(ParserTest, ExprBothAssoc) {
  MakeParser("a = b + c = d");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_EQ("(a ASSG ((b ADD c) ASSG d))", Str(expr.Get()));
}

TEST_F(ParserTest, ExprPrecedence) {
  MakeParser("a = b || c && d | e ^ f & g == h <= i + j * k");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_EQ("(a ASSG (b OR (c AND (d BOR (e XOR (f BAND (g EQ (h LE (i ADD (j MUL k))))))))))", Str(expr.Get()));
}

} // namespace parser
