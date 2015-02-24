#include "types/typechecker.h"

#include "lexer/lexer.h"
#include "parser/parser_internal.h"
#include "third_party/gtest/gtest.h"

using namespace ast;

using base::ErrorList;
using base::Pos;
using lexer::Token;
using parser::internal::Result;

#define EXPECT_ERRS(msg) EXPECT_EQ(msg, testing::PrintToString(errors_))
#define EXPECT_NO_ERRS EXPECT_EQ(0, errors_.Size())

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
  EXPECT_NO_ERRS;
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
  EXPECT_NO_ERRS;
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
  EXPECT_NO_ERRS;
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
  EXPECT_NO_ERRS;
}

TEST_F(TypeCheckerTest, BinExprNumericOpPromotion) {
  sptr<const Expr> before = ParseExpr("'3' + '3'");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
  EXPECT_NO_ERRS;
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
  EXPECT_NO_ERRS;
}

// TODO: CallExpr

// TODO: CastExpr

TEST_F(TypeCheckerTest, CharLitExpr) {
  sptr<const Expr> before = ParseExpr("'0'");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kChar, after->GetTypeId());
  EXPECT_NO_ERRS;
}

// TODO: FieldDerefExpr

// TODO: InstanceOfExpr

TEST_F(TypeCheckerTest, IntLitExpr) {
  sptr<const Expr> before = ParseExpr("0");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
  EXPECT_NO_ERRS;
}

// TODO: NameExpr

// TODO: NewArrayExpr

// TODO: NewClassExpr

TEST_F(TypeCheckerTest, NullLitExpr) {
  sptr<const Expr> before = ParseExpr("null");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kNull, after->GetTypeId());
  EXPECT_NO_ERRS;
}

// TODO: ParenExpr

// TODO: StringLitExpr

TEST_F(TypeCheckerTest, ThisLitExpr) {
  const auto insideType = TypeId{100, 0};

  sptr<const Expr> before = ParseExpr("this");

  auto typeChecker = (*typeChecker_.get())
    .InsideCompUnit(nullptr)
    .InsideTypeDecl(insideType)
    .InsideMethodDecl(TypeId::kVoid);

  auto after = typeChecker.Rewrite(before);

  EXPECT_EQ(insideType, after->GetTypeId());
  EXPECT_NO_ERRS;
}

// TODO: UnaryExpr


// TODO: BlockStmt

// TODO: EmptyStmt

// TODO: ExprStmt

// TODO: ForStmt

// TODO: IfStmt

// TODO: LocalDeclStmt

// TODO: ReturnStmt

// TODO: WhileStmt


// TODO: FieldDecl

// TODO: MethodDecl


// TODO: TypeDecl


// TODO: CompUnit


}  // namespace types
