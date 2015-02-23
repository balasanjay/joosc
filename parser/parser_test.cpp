#include "parser/parser_internal.h"

#include "ast/print_visitor.h"
#include "lexer/lexer.h"
#include "third_party/gtest/gtest.h"

using namespace ast;

using base::ErrorList;
using base::FileSet;
using base::PosRange;
using base::SharedPtrVector;
using lexer::DOT;
using lexer::K_THIS;
using lexer::Token;
using parser::internal::Result;

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
    base::ErrorList errors;

    // Create file set.
    base::FileSet* fs;
    ASSERT_TRUE(base::FileSet::Builder().AddStringFile("foo.joos", s).Build(
        &fs, &errors));
    fs_.reset(fs);

    // Lex tokens.
    vector<vector<lexer::Token>> alltokens;
    lexer::LexJoosFiles(fs, &alltokens, &errors);

    // Remote comments and whitespace.
    lexer::StripSkippableTokens(alltokens, &tokens);

    // Make sure it worked.
    ASSERT_EQ(1u, tokens.size());
    ASSERT_FALSE(errors.IsFatal());

    parser_.reset(new parser::Parser(fs, fs->Get(0), &tokens[0]));
  }

  vector<vector<Token>> tokens;
  uptr<FileSet> fs_;
  uptr<Parser> parser_;
};

template <typename T>
string Str(sptr<const T> t) {
  std::stringstream s;
  PrintVisitor visitor = PrintVisitor::Compact(&s);
  visitor.Rewrite(t);
  return s.str();
}

string TypeStr(sptr<const Type> type) {
  std::stringstream s;
  type->PrintTo(&s);
  return s.str();
}

string TypeStr(sptr<const QualifiedName> qn) {
  std::stringstream s;
  qn->PrintTo(&s);
  return s.str();
}

TEST_F(ParserTest, QualifiedNameNoLeadingIdent) {
  MakeParser(";");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(name));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n",
            testing::PrintToString(name.Errors()));
}

TEST_F(ParserTest, QualifiedNameSingleIdent) {
  MakeParser("foo");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(name));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(name.Errors().IsFatal());
  EXPECT_EQ("foo", TypeStr(name.Get()));
}

TEST_F(ParserTest, QualifiedNameMultiIdent) {
  MakeParser("foo.bar.baz");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(name));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(name.Errors().IsFatal());
  EXPECT_EQ("foo.bar.baz", TypeStr(name.Get()));
}

TEST_F(ParserTest, QualifiedNameTrailingDot) {
  MakeParser("foo.bar.baz.");

  Result<QualifiedName> name;
  Parser after = parser_->ParseQualifiedName(&name);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(name));
  EXPECT_TRUE(name.Errors().IsFatal());
  EXPECT_EQ("UnexpectedEOFError(0:11)\n",
            testing::PrintToString(name.Errors()));
}

TEST_F(ParserTest, SingleTypePrimitive) {
  MakeParser("int");

  Result<Type> type;
  Parser after = parser_->ParseSingleType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("K_INT", TypeStr(type.Get()));
}

TEST_F(ParserTest, SingleTypeReference) {
  MakeParser("String");

  Result<Type> type;
  Parser after = parser_->ParseSingleType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("String", TypeStr(type.Get()));
}

TEST_F(ParserTest, SingleTypeMultiReference) {
  MakeParser("java.lang.String");

  Result<Type> type;
  Parser after = parser_->ParseSingleType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("java.lang.String", TypeStr(type.Get()));
}

TEST_F(ParserTest, SingleTypeBothFail) {
  MakeParser(";");

  Result<Type> type;
  Parser after = parser_->ParseSingleType(&type);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(type));
  EXPECT_TRUE(type.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:0)\n",
            testing::PrintToString(type.Errors()));
}

TEST_F(ParserTest, TypeNonArray) {
  MakeParser("int");

  Result<Type> type;
  Parser after = parser_->ParseType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("K_INT", TypeStr(type.Get()));
}

TEST_F(ParserTest, TypeFail) {
  MakeParser(";");

  Result<Type> type;
  Parser after = parser_->ParseType(&type);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(type));
  EXPECT_TRUE(type.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:0)\n",
            testing::PrintToString(type.Errors()));
}

TEST_F(ParserTest, TypeArray) {
  MakeParser("int[]");

  Result<Type> type;
  Parser after = parser_->ParseType(&type);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(type));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_FALSE(type.Errors().IsFatal());
  EXPECT_EQ("array<K_INT>", TypeStr(type.Get()));
}

TEST_F(ParserTest, TypeArrayFail) {
  MakeParser("int[;");

  Result<Type> type;
  Parser after = parser_->ParseType(&type);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(type));
  EXPECT_TRUE(type.Errors().IsFatal());
  EXPECT_EQ("UnexpectedTokenError(0:4)\n",
            testing::PrintToString(type.Errors()));
}

TEST_F(ParserTest, ArgumentListNone) {
  MakeParser(")");
  Result<SharedPtrVector<const Expr>> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ(0, args.Get()->Size());
}

TEST_F(ParserTest, ArgumentListOne) {
  MakeParser("foo.bar");
  Result<SharedPtrVector<const Expr>> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ(1, args.Get()->Size());
}

TEST_F(ParserTest, ArgumentListMany) {
  MakeParser("a,b, c, d  , e");
  Result<SharedPtrVector<const Expr>> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
  EXPECT_EQ(5, args.Get()->Size());
}

TEST_F(ParserTest, ArgumentListHangingComma) {
  MakeParser("a, b,)");
  Result<SharedPtrVector<const Expr>> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(args));
}

TEST_F(ParserTest, ArgumentListNestedExpr) {
  MakeParser("a, (1 + b))");
  Result<SharedPtrVector<const Expr>> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
}

TEST_F(ParserTest, ArgumentListBadExpr) {
  MakeParser("a, ;)");
  Result<SharedPtrVector<const Expr>> args;
  Parser after = parser_->ParseArgumentList(&args);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(args));
  EXPECT_EQ("UnexpectedTokenError(0:3)\n",
            testing::PrintToString(args.Errors()));
}

TEST_F(ParserTest, ArgumentListStartingComma) {
  MakeParser(", a, b, c)");
  Result<SharedPtrVector<const Expr>> args;
  Parser after = parser_->ParseArgumentList(&args);
  // Shouldn't parse anything, since arg list is optional.
  // TODO: Do we actually want this to fail if it doesn't stop at at an
  // RPAREN?
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(args));
}

TEST_F(ParserTest, DISABLED_PrimaryBaseShortCircuit) {
  MakeParser("");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedEOFError(0:0)\n",
            testing::PrintToString(primary.Errors()));
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
  EXPECT_EQ("(INTEGER)", Str(primary.Get()));
}

TEST_F(ParserTest, PrimaryBaseParensExprFail) {
  MakeParser("(;)");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n",
            testing::PrintToString(primary.Errors()));
}

TEST_F(ParserTest, PrimaryBaseParensNoClosing) {
  MakeParser("(3;");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedTokenError(0:2)\n",
            testing::PrintToString(primary.Errors()));
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
  EXPECT_EQ("UnexpectedTokenError(0:4)\n",
            testing::PrintToString(primary.Errors()));
}

TEST_F(ParserTest, PrimaryBaseAbort) {
  MakeParser(";");
  Result<Expr> primary;
  Parser after = parser_->ParsePrimaryBase(&primary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primary));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n",
            testing::PrintToString(primary.Errors()));
}

TEST_F(ParserTest, PrimaryEndFailedArrayAccess) {
  MakeParser("[;]");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary, &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n",
            testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndArrayAccessWithField) {
  MakeParser("[3].f");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary, &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("this[INTEGER].f", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndArrayAccessNoTrailing) {
  MakeParser("[3]+5");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary, &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("this[INTEGER]", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndNoAccess) {
  MakeParser(".f");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary, &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("this.f", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, DISABLED_PrimaryEndNoArrayShortCircuit) {
  MakeParser("");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after =
      parser_->ParsePrimaryEndNoArrayAccess(primary, &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedEOFError(0:0)\n",
            testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndNoArrayUnexpectedToken) {
  MakeParser(";");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after =
      parser_->ParsePrimaryEndNoArrayAccess(primary, &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n",
            testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndNoArrayFieldFail) {
  MakeParser(".;");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after =
      parser_->ParsePrimaryEndNoArrayAccess(primary, &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n",
            testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndNoArrayFieldWithEnd) {
  MakeParser(".f[0]");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after =
      parser_->ParsePrimaryEndNoArrayAccess(primary, &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("this.f[INTEGER]", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndDoubleArrayAccess) {
  MakeParser("[0][1]");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after = parser_->ParsePrimaryEnd(primary, &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("this[INTEGER]", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndNoArrayFieldWithEndFail) {
  MakeParser(".f;");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after =
      parser_->ParsePrimaryEndNoArrayAccess(primary, &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("this.f", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndNoArrayMethodFail) {
  MakeParser("(;)");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after =
      parser_->ParsePrimaryEndNoArrayAccess(primary, &primaryEnd);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(primaryEnd));
  EXPECT_EQ("UnexpectedTokenError(0:1)\n",
            testing::PrintToString(primaryEnd.Errors()));
}

TEST_F(ParserTest, PrimaryEndNoArrayMethodWithEnd) {
  MakeParser("().f");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after =
      parser_->ParsePrimaryEndNoArrayAccess(primary, &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("this().f", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, PrimaryEndNoArrayMethodWithEndFail) {
  MakeParser("();");
  sptr<const Expr> primary(new ThisExpr(Token(K_THIS, PosRange(0, 0, 4))));
  Result<Expr> primaryEnd;
  Parser after =
      parser_->ParsePrimaryEndNoArrayAccess(primary, &primaryEnd);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(primaryEnd));
  EXPECT_EQ("this()", Str(primaryEnd.Get()));
}

TEST_F(ParserTest, DISABLED_UnaryEmptyShortCircuit) {
  MakeParser("");

  Result<Expr> unary;
  Parser after = parser_->ParseUnaryExpression(&unary);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unary));
  EXPECT_EQ("UnexpectedEOFError(0:0)\n",
            testing::PrintToString(unary.Errors()));
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
  EXPECT_EQ("UnexpectedTokenError(0:1)\n",
            testing::PrintToString(unary.Errors()));
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
  EXPECT_EQ("UnexpectedTokenError(0:1)\n",
            testing::PrintToString(cast.Errors()));
}

TEST_F(ParserTest, CastExprFail) {
  MakeParser("(int) ;");

  Result<Expr> cast;
  Parser after = parser_->ParseCastExpression(&cast);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(cast));
  EXPECT_EQ("UnexpectedTokenError(0:6)\n",
            testing::PrintToString(cast.Errors()));
}

TEST_F(ParserTest, InstanceOfRefType) {
  MakeParser("a instanceof String");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_EQ("(a instanceof String)", Str(expr.Get()));
}

TEST_F(ParserTest, InstanceOfArray) {
  MakeParser("a instanceof int[]");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(expr));
  EXPECT_EQ("(a instanceof array<K_INT>)", Str(expr.Get()));
}

TEST_F(ParserTest, InstanceOfParens) {
  MakeParser("a instanceof (String)");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_EQ("UnexpectedTokenError(0:13)\n",
            testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, InstanceOfNull) {
  MakeParser("a instanceof null");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_EQ("UnexpectedTokenError(0:13)\n",
            testing::PrintToString(expr.Errors()));
}

TEST_F(ParserTest, ExprUnaryFail) {
  MakeParser(";");

  Result<Expr> expr;
  Parser after = parser_->ParseExpression(&expr);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(expr));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n",
            testing::PrintToString(expr.Errors()));
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
  EXPECT_EQ("UnexpectedTokenError(0:3)\n",
            testing::PrintToString(expr.Errors()));
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
  EXPECT_EQ(
      "(a ASSG (b OR (c AND (d BOR (e XOR (f BAND (g EQ (h LE (i ADD (j MUL "
      "k))))))))))",
      Str(expr.Get()));
}

TEST_F(ParserTest, VarDecl) {
  MakeParser("java.lang.Integer foobar = 1");

  Result<Stmt> stmt;
  Parser after = parser_->ParseVarDecl(&stmt);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("java.lang.Integer foobar=INTEGER;", Str(stmt.Get()));
}

TEST_F(ParserTest, VarDeclBadIdentifier) {
  MakeParser("java.lang.Integer int = 1");

  Result<Stmt> stmt;
  Parser after = parser_->ParseVarDecl(&stmt);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:18)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, VarDeclBadNoAssign) {
  MakeParser("java.lang.Integer foo;");

  Result<Stmt> stmt;
  Parser after = parser_->ParseVarDecl(&stmt);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:21)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, VarDeclBadAssign) {
  MakeParser("java.lang.Integer foo = ;");

  Result<Stmt> stmt;
  Parser after = parser_->ParseVarDecl(&stmt);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:24)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ReturnStmtEmpty) {
  MakeParser("return;");

  Result<Stmt> stmt;
  Parser after = parser_->ParseReturnStmt(&stmt);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("return;", Str(stmt.Get()));
}

TEST_F(ParserTest, ReturnStmtNoSemi) {
  MakeParser("return");

  Result<Stmt> stmt;
  Parser after = parser_->ParseReturnStmt(&stmt);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedEOFError(0:5)\n", testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ReturnStmtWithExprNoSemi) {
  MakeParser("return 1");

  Result<Stmt> stmt;
  Parser after = parser_->ParseReturnStmt(&stmt);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedEOFError(0:7)\n", testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ReturnStmtWithExpr) {
  MakeParser("return 1;");

  Result<Stmt> stmt;
  Parser after = parser_->ParseReturnStmt(&stmt);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("return INTEGER;", Str(stmt.Get()));
}

TEST_F(ParserTest, ReturnStmtBadExpr) {
  MakeParser("return (;);");

  Result<Stmt> stmt;
  Parser after = parser_->ParseReturnStmt(&stmt);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:8)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, BlockStmtEmpty) {
  MakeParser("{}");

  Result<Stmt> stmt;
  Parser after = parser_->ParseBlock(&stmt);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("{}", Str(stmt.Get()));
}

TEST_F(ParserTest, BlockStmtSemis) {
  MakeParser("{;;;;;;;}");

  Result<Stmt> stmt;
  Parser after = parser_->ParseBlock(&stmt);

  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("{;;;;;;;}", Str(stmt.Get()));
}

TEST_F(ParserTest, BlockStmtNoSemi) {
  MakeParser("{foo}");

  Result<Stmt> stmt;
  Parser after = parser_->ParseBlock(&stmt);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:4)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, BlockStmtNestedNoClose) {
  MakeParser("{{{}}{;}");

  Result<Stmt> stmt;
  Parser after = parser_->ParseBlock(&stmt);

  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedEOFError(0:7)\n", testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, BlockStmtNested) {
  MakeParser("{a;\n{\nb;\n}\n;\n}");

  Result<Stmt> stmt;
  Parser after = parser_->ParseBlock(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("{a;{b;};}", Str(stmt.Get()));
}

TEST_F(ParserTest, IfStmtElse) {
  MakeParser("if(true)foo;else bar;");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("if(K_TRUE){foo;}else{bar;}", Str(stmt.Get()));
}

TEST_F(ParserTest, IfStmtElseBlock) {
  MakeParser("if(true)foo;else{bar;}");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("if(K_TRUE){foo;}else{{bar;}}", Str(stmt.Get()));
}

TEST_F(ParserTest, IfStmtTooManyElses) {
  MakeParser("if(true)foo;else{bar;}else{baz;}");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("if(K_TRUE){foo;}else{{bar;}}", Str(stmt.Get()));
}

TEST_F(ParserTest, IfStmtHangingElse) {
  MakeParser("if(a) if(b) c; else d;");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("if(a){if(b){c;}else{d;}}else{;}", Str(stmt.Get()));
}

TEST_F(ParserTest, IfStmtOutsideElse) {
  MakeParser("if(a){if(b) c;}else d;");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("if(a){{if(b){c;}else{;}}}else{d;}", Str(stmt.Get()));
}

TEST_F(ParserTest, IfStmtFailBodyDecl) {
  MakeParser("if(a) string b = 1;");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
}

TEST_F(ParserTest, IfStmtIfIfElseElse) {
  MakeParser("if(a)if(b)foo();else bar();else baz();");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("if(a){if(b){foo();}else{bar();}}else{baz();}", Str(stmt.Get()));
}

TEST_F(ParserTest, IfStmtElseIf) {
  MakeParser("if(a)foo();else if(b)bar();");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("if(a){foo();}else{if(b){bar();}else{;}}", Str(stmt.Get()));
}

TEST_F(ParserTest, IfStmtElseIfElse) {
  MakeParser("if(a)foo();else if(b)bar();else baz();");

  Result<Stmt> stmt;
  Parser after = parser_->ParseIfStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("if(a){foo();}else{if(b){bar();}else{baz();}}", Str(stmt.Get()));
}

TEST_F(ParserTest, ForInitDecl) {
  MakeParser("int a = 1");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForInit(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
}

TEST_F(ParserTest, ForInitAssign) {
  MakeParser("a = 1");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForInit(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
}

TEST_F(ParserTest, ForInitNewClass) {
  MakeParser("new Foo(1)");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForInit(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
}

TEST_F(ParserTest, ForInitNoIf) {
  MakeParser("if(a)b");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForInit(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtEmpty) {
  MakeParser("for(;;);");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("for(;;){;}", Str(stmt.Get()));
}

TEST_F(ParserTest, ForStmtBlock) {
  MakeParser("for(;;){a;}");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("for(;;){{a;}}", Str(stmt.Get()));
}

TEST_F(ParserTest, ForStmtFull) {
  MakeParser("for(i=1;i;i) print(i);");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("for((i ASSG INTEGER);i;i){print(i);}", Str(stmt.Get()));
}

TEST_F(ParserTest, ForStmtBadCond) {
  MakeParser("for(i=1;int i=2;i) print(i);");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:8)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtBadInit) {
  MakeParser("for(if(i)i;;);");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:4)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtTooManyStatements) {
  MakeParser("for(;;;);");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:6)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtUnclosed) {
  MakeParser("for(;;{;}");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:6)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtTooFewStmts) {
  MakeParser("for(;)");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:5)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtNoStmts) {
  MakeParser("for()");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:4)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtNoParen) {
  MakeParser("for;");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:3)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtPropagateErrorFromInit) {
  MakeParser("for(a+;;);");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:6)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtPropagateErrorFromCond) {
  MakeParser("for(;a+;);");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:7)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtPropagateErrorFromUpdate) {
  MakeParser("for(;;a+);");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:8)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, ForStmtPropagateErrorFromBody) {
  MakeParser("for(;;)a+;");
  Result<Stmt> stmt;
  Parser after = parser_->ParseForStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:9)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, WhileStmtNoWhile) {
  MakeParser("asdf");
  Result<Stmt> stmt;
  Parser after = parser_->ParseWhileStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:0)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, WhileStmtNoLParen) {
  MakeParser("while{");
  Result<Stmt> stmt;
  Parser after = parser_->ParseWhileStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:5)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, WhileStmtNoCond) {
  MakeParser("while()");
  Result<Stmt> stmt;
  Parser after = parser_->ParseWhileStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:6)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, WhileStmtBadCond) {
  MakeParser("while(;)");
  Result<Stmt> stmt;
  Parser after = parser_->ParseWhileStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:6)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, WhileStmtNoRParen) {
  MakeParser("while(1}");
  Result<Stmt> stmt;
  Parser after = parser_->ParseWhileStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:7)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, WhileStmtNoBody) {
  MakeParser("while(1)");
  Result<Stmt> stmt;
  Parser after = parser_->ParseWhileStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedEOFError(0:7)\n", testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, WhileStmtBadBody) {
  MakeParser("while(1)class");
  Result<Stmt> stmt;
  Parser after = parser_->ParseWhileStmt(&stmt);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(stmt));
  EXPECT_EQ("UnexpectedTokenError(0:8)\n",
            testing::PrintToString(stmt.Errors()));
}

TEST_F(ParserTest, WhileStmtSuccess) {
  MakeParser("while(1){int i = 0;}");
  Result<Stmt> stmt;
  Parser after = parser_->ParseWhileStmt(&stmt);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(stmt));
  EXPECT_EQ("while(INTEGER){{K_INT i=INTEGER;}}", Str(stmt.Get()));
}

TEST_F(ParserTest, ParamListBasic) {
  MakeParser("int a, String b, a.b.c.d.e foo");
  Result<ParamList> params;
  Parser after = parser_->ParseParamList(&params);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(params));
  EXPECT_EQ("K_INT a,String b,a.b.c.d.e foo",
            Str(params.Get()));
}

TEST_F(ParserTest, ParamListOne) {
  MakeParser("int a");
  Result<ParamList> params;
  Parser after = parser_->ParseParamList(&params);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(params));
  EXPECT_EQ("K_INT a", Str(params.Get()));
}

TEST_F(ParserTest, ParamListEmpty) {
  MakeParser(")");
  Result<ParamList> params;
  Parser after = parser_->ParseParamList(&params);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(params));
  EXPECT_EQ("", Str(params.Get()));
}

TEST_F(ParserTest, ParamListNoParamName) {
  MakeParser("int");
  Result<ParamList> params;
  Parser after = parser_->ParseParamList(&params);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(params));
  EXPECT_EQ("ParamRequiresNameError(0:0-3)\n",
            testing::PrintToString(params.Errors()));
}

TEST_F(ParserTest, ParamListHangingComma) {
  MakeParser("int foo,)");
  Result<ParamList> params;
  Parser after = parser_->ParseParamList(&params);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(params));
  EXPECT_EQ("UnexpectedTokenError(0:8)\n",
            testing::PrintToString(params.Errors()));
}

TEST_F(ParserTest, ParamListHangingCommaEOF) {
  MakeParser("int foo,");
  Result<ParamList> params;
  Parser after = parser_->ParseParamList(&params);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(params));
  EXPECT_EQ("UnexpectedEOFError(0:7)\n",
            testing::PrintToString(params.Errors()));
}

TEST_F(ParserTest, FieldDeclSimple) {
  MakeParser("int foo;");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_EQ("K_INT foo;", Str(decl.Get()));
}

TEST_F(ParserTest, FieldDeclModsOrdered) {
  MakeParser("native public static abstract protected final int foo;");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_EQ(
      "K_PUBLIC K_PROTECTED K_ABSTRACT K_STATIC K_FINAL K_NATIVE K_INT "
      "foo;",
      Str(decl.Get()));
}

TEST_F(ParserTest, FieldDeclWithAssign) {
  MakeParser("int foo = 1;");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_EQ("K_INT foo=INTEGER;", Str(decl.Get()));
}

TEST_F(ParserTest, FieldDeclExprError) {
  MakeParser("int foo = -1 +;");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:14)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, FieldDeclJustEq) {
  MakeParser("int foo =;");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:9)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, FieldDeclBadBlock) {
  MakeParser("int foo{}");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:7)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, FieldDeclNoSemi) {
  MakeParser("int foo = 1}");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:11)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, MethodDeclNoBody) {
  MakeParser("int foo();");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_EQ("K_INT foo();", Str(decl.Get()));
}

TEST_F(ParserTest, MethodDeclParamsBlock) {
  MakeParser("public int main(int argc, String[] argv) {foo;}");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_EQ(
      "K_PUBLIC K_INT main(K_INT argc,array<String> "
      "argv){foo;}",
      Str(decl.Get()));
}

TEST_F(ParserTest, MethodConstDeclNoBody) {
  MakeParser("foo();");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_EQ("foo();", Str(decl.Get()));
}

TEST_F(ParserTest, MethodConstDeclBody) {
  MakeParser("foo() { a; }");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_EQ("foo(){a;}", Str(decl.Get()));
}

TEST_F(ParserTest, MethodConstDeclMembers) {
  MakeParser("foo(int a, int b) {}");
  Result<MemberDecl> decl;
  Parser after = parser_->ParseMemberDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_EQ("foo(K_INT a,K_INT b){}", Str(decl.Get()));
}

TEST_F(ParserTest, TypeDeclBadModifierList) {
  MakeParser("public public");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("DuplicateModifierError(0:7-13)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclEOFAfterMods) {
  MakeParser("public");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedEOFError(0:5)\n", testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclNoType) {
  MakeParser("public 3");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:7)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclNoIdent) {
  MakeParser("public class 3");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:13)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclClassBadSuper) {
  MakeParser("public class Foo extends 123");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:25)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclClassBadImplements) {
  MakeParser("public class Foo extends Bar implements 123");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:40)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclClassBadImplementsList) {
  MakeParser("public class Foo extends Bar implements Baz, 123");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:45)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclClassNoLBrace) {
  MakeParser("public class Foo extends Bar implements Baz, Buh (");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:49)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclClassBadMember) {
  MakeParser("public class Foo extends Bar implements Baz, Buh {3;}");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:50)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclClassManySemis) {
  MakeParser(
      "public class Foo extends Bar implements Baz, Buh {;;;;;;;int i = 0;}");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_EQ(
      "K_PUBLIC class Foo extends Bar implements Baz,Buh {K_INT "
      "i=INTEGER;}",
      Str(decl.Get()));
}

TEST_F(ParserTest, TypeDeclInterfaceBadExtends) {
  MakeParser("public interface Foo extends 123");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:29)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclInterfaceBadExtendsList) {
  MakeParser("public interface Foo extends Bar, 123");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(decl));
  EXPECT_EQ("UnexpectedTokenError(0:34)\n",
            testing::PrintToString(decl.Errors()));
}

TEST_F(ParserTest, TypeDeclInterfaceManySemis) {
  MakeParser("public interface Foo extends Bar, Baz, Buh {;;;;;;;int i = 0;}");
  Result<TypeDecl> decl;
  Parser after = parser_->ParseTypeDecl(&decl);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(decl));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_EQ(
      "K_PUBLIC interface Foo extends Bar,Baz,Buh {K_INT "
      "i=INTEGER;}",
      Str(decl.Get()));
}

TEST_F(ParserTest, CompUnitEmptyFile) {
  MakeParser("");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unit));
  EXPECT_TRUE(after.IsAtEnd());
  EXPECT_EQ("", Str(unit.Get()));
}

TEST_F(ParserTest, CompUnitBadPackage) {
  MakeParser("package 1;");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedTokenError(0:8)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitBadImport) {
  MakeParser("package foo; import 1");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedTokenError(0:20)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitBadImports) {
  MakeParser("package foo; import bar; import 1");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedTokenError(0:32)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitBadImportTooManyStars) {
  MakeParser("package foo; import bar.*.*;");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedTokenError(0:25)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitBadImportBadIdentAfterDot) {
  MakeParser("package foo; import bar.baz.1;");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedTokenError(0:28)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitBadImportEOF) {
  MakeParser("package foo; import bar.baz");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedEOFError(0:26)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitBadImportNoSemi) {
  MakeParser("package foo; import bar.baz,");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedTokenError(0:27)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitBadType) {
  MakeParser("package foo; import bar.baz; public int i;");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedTokenError(0:36)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitExtraTokens) {
  MakeParser("package foo; import bar.baz; public class foo{} i");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_FALSE(b(after));
  EXPECT_FALSE(b(unit));
  EXPECT_EQ("UnexpectedTokenError(0:48)\n",
            testing::PrintToString(unit.Errors()));
}

TEST_F(ParserTest, CompUnitSuccess) {
  MakeParser("package foo; import bar.baz.*; public class foo{}");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unit));
  EXPECT_EQ("package foo;import bar.baz.*;K_PUBLIC class foo {}",
            Str(unit.Get()));
}

TEST_F(ParserTest, CompUnitOnlyPackageSuccess) {
  MakeParser("package foo;");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unit));
  EXPECT_EQ("package foo;", Str(unit.Get()));
}

TEST_F(ParserTest, CompUnitOnlyImportSuccess) {
  MakeParser("import foo;");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unit));
  EXPECT_EQ("import foo;", Str(unit.Get()));
}

TEST_F(ParserTest, CompUnitOnlyTypeSuccess) {
  MakeParser("class Foo{}");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unit));
  EXPECT_EQ("class Foo {}", Str(unit.Get()));
}

TEST_F(ParserTest, CompUnitOnlySemiSuccess) {
  MakeParser(";");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unit));
  EXPECT_EQ("", Str(unit.Get()));
}

TEST_F(ParserTest, CompUnitManySemisSuccess) {
  MakeParser(
      "package foo;;;;;;;;;;;;;;import bar.baz.*;;;;;;;;;;;public class "
      "foo{};;;;;;;");
  Result<CompUnit> unit;
  Parser after = parser_->ParseCompUnit(&unit);
  EXPECT_TRUE(b(after));
  EXPECT_TRUE(b(unit));
  EXPECT_EQ("package foo;import bar.baz.*;K_PUBLIC class foo {}",
            Str(unit.Get()));
}

}  // namespace parser
