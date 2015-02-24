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
  EXPECT_EQ("UnaryNonNumericError(0:1-6)\n", testing::PrintToString(errors_));
}

TEST_F(TypeCheckerTest, BinExprRhsFail) {
  sptr<const Expr> before = ParseExpr("3 + (-null)");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_EQ("UnaryNonNumericError(0:5-10)\n", testing::PrintToString(errors_));
}

TEST_F(TypeCheckerTest, BinExprBoolOp) {
  sptr<const Expr> before = ParseExpr("true || false");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kBool, after->GetTypeId());
}

// TODO: BoolLitExpr

// TODO: CallExpr

// TODO: CastExpr

// TODO: CharLitExpr

// TODO: FieldDerefExpr

// TODO: InstanceOfExpr

// TODO: IntLitExpr

// TODO: NameExpr

// TODO: NewArrayExpr

// TODO: NewClassExpr

// TODO: NullLitExpr

// TODO: ParenExpr

// TODO: StringLitExpr

// TODO: ThisExpr

// TODO: UnaryExpr


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
}

// TODO: FieldDecl

// TODO: MethodDecl


// TODO: TypeDecl


// TODO: CompUnit


}  // namespace types
