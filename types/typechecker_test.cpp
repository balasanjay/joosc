#include "types/typechecker.h"

#include "base/file.h"
#include "ast/ids.h"
#include "lexer/lexer.h"
#include "parser/parser_internal.h"
#include "third_party/gtest/gtest.h"

using namespace ast;

using base::ErrorList;
using base::Pos;
using base::PosRange;
using lexer::Token;
using parser::internal::Result;

#define EXPECT_ERRS(msg) EXPECT_EQ(msg, testing::PrintToString(errors_))
#define EXPECT_NO_ERRS() EXPECT_EQ(0, errors_.Size())

namespace types {

class TypeCheckerTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    typeChecker_.reset();
    parser_.reset();
    tokens.clear();
    fs_.reset();
    errors_.Clear();
  }

  virtual void TearDown() {
    typeChecker_.reset();
    parser_.reset();
    tokens.clear();
    fs_.reset();
    errors_.Clear();
  }

  void MakeParser(string s) {
    // Create file set.
    base::FileSet* fs;
    ASSERT_TRUE(base::FileSet::Builder().AddStringFile("Foo.java", s).Build(
        &fs, &errors_));
    fs_.reset(fs);

    // Lex tokens.
    vector<vector<lexer::Token>> alltokens;
    lexer::LexJoosFiles(fs, &alltokens, &errors_);

    // Remote comments and whitespace.
    lexer::StripSkippableTokens(alltokens, &tokens);

    // Make sure it worked.
    ASSERT_EQ(1u, tokens.size());
    ASSERT_FALSE(errors_.IsFatal());
    ASSERT_EQ(0, errors_.Size());

    parser_.reset(new parser::Parser(fs, fs->Get(0), &tokens[0]));

    typeChecker_.reset(new TypeChecker(fs_.get(), &errors_));
  }

  sptr<const Expr> ParseExpr(string s) {
    MakeParser(s);
    Result<Expr> exprResult;
    assert(!parser_->ParseExpression(&exprResult).Failed());
    return exprResult.Get();
  }

  sptr<const Stmt> ParseStmt(string s) {
    MakeParser(s);
    Result<Stmt> stmtResult;
    assert(!parser_->ParseStmt(&stmtResult).Failed());
    return stmtResult.Get();
  }

  base::ErrorList errors_;
  uptr<base::FileSet> fs_;
  vector<vector<lexer::Token>> tokens;
  uptr<parser::Parser> parser_;
  uptr<TypeChecker> typeChecker_;
};

// TODO: ArrayIndexExpr

// TODO: BinExpr

TEST_F(TypeCheckerTest, BinExprLhsFail) {
  sptr<const Expr> before = ParseExpr("(-null) + 3");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("UnaryNonNumericError(0:1-6)\n");
}

TEST_F(TypeCheckerTest, BinExprRhsFail) {
  sptr<const Expr> before = ParseExpr("3 + (-null)");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("UnaryNonNumericError(0:5-10)\n");
}

TEST_F(TypeCheckerTest, BinExprBoolOpSuccess) {
  sptr<const Expr> before = ParseExpr("true || false");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kBool, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, BinExprBoolOpOperandsNotBool) {
  sptr<const Expr> before = ParseExpr("3 && 3");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:0)\nTypeMismatchError(0:5)\n");
}

TEST_F(TypeCheckerTest, BinExprRelationalOpSuccess) {
  sptr<const Expr> before = ParseExpr("3 < 4");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kBool, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, BinExprRelationalOperandsNotNumeric) {
  sptr<const Expr> before = ParseExpr("true >= false");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:0-4)\nTypeMismatchError(0:8-13)\n");
}

TEST_F(TypeCheckerTest, BinExprEqualityOpSuccess) {
  sptr<const Expr> before = ParseExpr("3 == 3");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kBool, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, BinExprEqualityOpIncomparable) {
  sptr<const Expr> before = ParseExpr("true != 3");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("IncomparableTypeError(0:5-7)\n");
}

// TODO: Need fake TypeSet to test String concatenation.

TEST_F(TypeCheckerTest, BinExprNumericOpSuccess) {
  sptr<const Expr> before = ParseExpr("3 + 3");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, BinExprNumericOpPromotion) {
  sptr<const Expr> before = ParseExpr("'3' + '3'");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, BinExprNumericOpOperandsNotNumeric) {
  sptr<const Expr> before = ParseExpr("true - null");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:0-4)\nTypeMismatchError(0:7-11)\n");
}

TEST_F(TypeCheckerTest, BoolLitExpr) {
  sptr<const Expr> before = ParseExpr("true");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kBool, after->GetTypeId());
  EXPECT_NO_ERRS();
}

// TODO: CallExpr

TEST_F(TypeCheckerTest, CastExprNullptr) {
  sptr<const Expr> before = ParseExpr("(int)(1 + null)");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(after, nullptr);
  EXPECT_ERRS("TypeMismatchError(0:10-14)\n");
}

TEST_F(TypeCheckerTest, CastExprTypeNullptr) {
  sptr<const Expr> before = ParseExpr("(foo)1");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(after, nullptr);
  EXPECT_ERRS("UnknownTypenameError(0:1-4)\n");
}

TEST_F(TypeCheckerTest, CastExprNotCastable) {
  sptr<const Expr> before = ParseExpr("(int)true");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(after, nullptr);
  EXPECT_ERRS("IncompatibleCastError(0:0-9)\n");
}

TEST_F(TypeCheckerTest, CastExprCastable) {
  sptr<const Expr> before = ParseExpr("(int)1");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
  EXPECT_NO_ERRS();
}

// TODO: Cast expr tests with Reference type as LHS.

TEST_F(TypeCheckerTest, CharLitExpr) {
  sptr<const Expr> before = ParseExpr("'0'");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kChar, after->GetTypeId());
  EXPECT_NO_ERRS();
}

// TODO: FieldDerefExpr

// TODO: InstanceOfExpr

TEST_F(TypeCheckerTest, IntLitExpr) {
  sptr<const Expr> before = ParseExpr("0");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
  EXPECT_NO_ERRS();
}

// TODO: NameExpr

// TODO: NewArrayExpr

// TODO: NewClassExpr

TEST_F(TypeCheckerTest, NullLitExpr) {
  sptr<const Expr> before = ParseExpr("null");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kNull, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, ParenExprIntInside) {
  sptr<const Expr> before = ParseExpr("(1+2)");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
}

TEST_F(TypeCheckerTest, ParenExprErrorInside) {
  sptr<const Expr> before = ParseExpr("(null-1)");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:1-5)\n");
}

// TODO: StringLitExpr

TEST_F(TypeCheckerTest, ThisLitExpr) {
  const auto insideType = TypeId{100, 0};

  sptr<const Expr> before = ParseExpr("this");

  auto typeChecker = (*typeChecker_.get())
    .InsideCompUnit(nullptr)
    .InsideTypeDecl(insideType)
    .InsideMemberDecl(TypeId::kVoid, false, ParamList({}));

  auto after = typeChecker.Rewrite(before);

  EXPECT_EQ(insideType, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, ThisLitExprInStaticMethod) {
  const auto insideType = TypeId{100, 0};

  sptr<const Expr> before = ParseExpr("this");

  auto typeChecker = (*typeChecker_.get())
    .InsideCompUnit(nullptr)
    .InsideTypeDecl(insideType)
    .InsideMemberDecl(TypeId::kVoid, true, ParamList({}));

  auto after = typeChecker.Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("ThisInStaticMemberError(0:0-4)\n");
}

TEST_F(TypeCheckerTest, UnaryExprErrorFromRHS) {
  sptr<const Expr> before = ParseExpr("!(null-1)");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:2-6)\n");
}

TEST_F(TypeCheckerTest, UnaryExprSubNonNumeric) {
  sptr<const Expr> before = ParseExpr("-true");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("UnaryNonNumericError(0:0-5)\n");
}

TEST_F(TypeCheckerTest, UnaryExprSubNumeric) {
  sptr<const Expr> before = ParseExpr("-'a'");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
}

TEST_F(TypeCheckerTest, UnaryExprNotNonBool) {
  sptr<const Expr> before = ParseExpr("!1");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("UnaryNonBoolError(0:0-2)\n");
}

TEST_F(TypeCheckerTest, UnaryExprNotIsBool) {
  sptr<const Expr> before = ParseExpr("!false");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kBool, after->GetTypeId());
}

// TODO: BlockStmt

// TODO: EmptyStmt

// TODO: ExprStmt

TEST_F(TypeCheckerTest, ForStmtInitError) {
  sptr<const Stmt> before = ParseStmt("for (boolean i = 1; 1 < 2;);");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("UnassignableError(0:17)\n");
}

// TODO: error in body statement.

TEST_F(TypeCheckerTest, ForStmtCondError) {
  sptr<const Stmt> before = ParseStmt("for (int i = 1;1 + null;);");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_ERRS("TypeMismatchError(0:19-23)\n");
}

TEST_F(TypeCheckerTest, ForStmtUpdateError) {
  sptr<const Stmt> before = ParseStmt("for (int i = 1;;1 + null);");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_ERRS("TypeMismatchError(0:20-24)\n");
}

TEST_F(TypeCheckerTest, ForStmtCondNotBool) {
  sptr<const Stmt> before = ParseStmt("for (int i = 1;1 + 1;);");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:15-20)\n");
}

TEST_F(TypeCheckerTest, ForStmtOk) {
  sptr<const Stmt> before = ParseStmt("for (int i = 1; 1 == 1; );");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_NE(nullptr, after);
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, IfStmtCondError) {
  sptr<const Stmt> before = ParseStmt("if(1 + null);");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:7-11)\n");
}

// TODO: errors in trueBody and falseBody statements.

TEST_F(TypeCheckerTest, IfStmtCondNotBool) {
  sptr<const Stmt> before = ParseStmt("if(1 + 1);");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:3-8)\n");
}

TEST_F(TypeCheckerTest, IfStmtOk) {
  sptr<const Stmt> before = ParseStmt("if(true) {} else {}");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_NE(nullptr, after);
  EXPECT_NO_ERRS();
}

// TODO: LocalDeclStmt

// TODO: ReturnStmt

TEST_F(TypeCheckerTest, WhileStmtCondError) {
  sptr<const Stmt> before = ParseStmt("while(true + 1);");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:6-10)\n");
}

// TODO: error in body statement.

TEST_F(TypeCheckerTest, WhileStmtCondNotBool) {
  sptr<const Stmt> before = ParseStmt("while(1);");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("TypeMismatchError(0:6)\n");
}

TEST_F(TypeCheckerTest, WhileStmtOk) {
  sptr<const Stmt> before = ParseStmt("while(true){}");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_NE(nullptr, after);
  EXPECT_NO_ERRS();
}

// TODO: FieldDecl

// TODO: MethodDecl


// TODO: TypeDecl


// TODO: CompUnit


TEST(TypeCheckerUtilTest, IsCastablePrimitives) {
  TypeChecker typeChecker(nullptr, nullptr);
  auto numTids = {TypeId::kInt, TypeId::kChar, TypeId::kShort, TypeId::kByte};
  for (TypeId tidA : numTids) {
    for (TypeId tidB : numTids) {
      EXPECT_TRUE(typeChecker.IsCastable(tidA, tidB));
    }
  }
  EXPECT_TRUE(typeChecker.IsCastable(TypeId::kBool, TypeId::kBool));
}

// TODO: TEST(TypeCheckerUtilTest, IsCastableReference) - with inheritance.

}  // namespace types
