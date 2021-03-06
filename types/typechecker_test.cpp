#include "types/typechecker.h"

#include "ast/ids.h"
#include "base/file.h"
#include "gtest/gtest.h"
#include "lexer/lexer.h"
#include "parser/parser_internal.h"
#include "types/types_test.h"

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

    // Remove comments and whitespace.
    lexer::StripSkippableTokens(alltokens, &tokens);

    // Make sure it worked.
    ASSERT_EQ(1u, tokens.size());
    ASSERT_FALSE(errors_.IsFatal());
    ASSERT_EQ(0, errors_.Size());

    parser_.reset(new parser::Parser(fs, fs->Get(0), 0, &tokens[0]));

    typeChecker_.reset(new TypeChecker(&errors_));
  }

  sptr<const Expr> ParseExpr(string s) {
    MakeParser(s);
    Result<Expr> exprResult;
    CHECK(!parser_->ParseExpression(&exprResult).Failed());
    return exprResult.Get();
  }

  sptr<const Stmt> ParseStmt(string s) {
    MakeParser(s);
    Result<Stmt> stmtResult;
    CHECK(!parser_->ParseStmt(&stmtResult).Failed());
    return stmtResult.Get();
  }

  sptr<const MemberDecl> ParseMemberDecl(string s) {
    MakeParser(s);
    Result<MemberDecl> memberResult;
    CHECK(!parser_->ParseMemberDecl(&memberResult).Failed());
    return memberResult.Get();
  }

  // Pairs of file name, file contents.
  sptr<const Program> ParseProgram(const vector<pair<string, string>>& file_contents) {
    base::FileSet* fs;
    sptr<const Program> program = ParseProgramWithStdlib(&fs, file_contents, &errors_);
    fs_.reset(fs);
    return program;
  }

  base::ErrorList errors_;
  uptr<base::FileSet> fs_;
  vector<vector<lexer::Token>> tokens;
  uptr<parser::Parser> parser_;
  uptr<TypeChecker> typeChecker_;
};

// TODO: ArrayIndexExpr

TEST_F(TypeCheckerTest, BinExprLhsFail) {
  sptr<const Expr> before = ParseExpr("(-null) + 3");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("UnaryNonNumericError(0:1-6)\n");
}

TEST_F(TypeCheckerTest, BinExprLhsVoid) {
  ParseProgram({
    {"A.java", "public class A { public void foo() {} public A() { int i = foo() + 1; } }"},
  });
  EXPECT_ERRS("VoidInExprError(0:59-64)\n");
}

TEST_F(TypeCheckerTest, BinExprRhsFail) {
  sptr<const Expr> before = ParseExpr("3 + (-null)");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("UnaryNonNumericError(0:5-10)\n");
}

TEST_F(TypeCheckerTest, BinExprRhsVoid) {
  ParseProgram({
    {"A.java", "public class A { public void foo() {} public A() { int i = 1 + foo(); } }"},
  });
  EXPECT_ERRS("VoidInExprError(0:63-68)\n");
}

TEST_F(TypeCheckerTest, BinExprBoolOpSuccess) {
  sptr<const Expr> before = ParseExpr("true || false");
  auto after = typeChecker_->Rewrite(before);

  ASSERT_NE(nullptr, after);
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

TEST_F(TypeCheckerTest, BinExprAssignment) {
  sptr<const Expr> before = ParseExpr("a = 1");

  const auto insideType = TypeId{100, 0};
  auto typeChecker = (*typeChecker_.get())
    .InsideCompUnit(nullptr)
    .InsideTypeDecl(insideType)
    .InsideMemberDecl(false, TypeId::kVoid, {{TypeId::kInt, "a", PosRange(0, 0, 1)}});

  auto after = typeChecker.Rewrite(before);
  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, BinExprAssignmentFails) {
  sptr<const Expr> before = ParseExpr("a = true");

  const auto insideType = TypeId{100, 0};
  auto typeChecker = (*typeChecker_.get())
    .InsideCompUnit(nullptr)
    .InsideTypeDecl(insideType)
    .InsideMemberDecl(false, TypeId::kVoid, {{TypeId::kInt, "a", PosRange(0, 0, 1)}});

  auto after = typeChecker.Rewrite(before);
  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("UnassignableError(0:4-8)\n");
}

TEST_F(TypeCheckerTest, BoolLitExpr) {
  sptr<const Expr> before = ParseExpr("true");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kBool, after->GetTypeId());
  EXPECT_NO_ERRS();
}

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

TEST_F(TypeCheckerTest, CharLitExpr) {
  sptr<const Expr> before = ParseExpr("'0'");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kChar, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, IntLitExpr) {
  sptr<const Expr> before = ParseExpr("0");
  auto after = typeChecker_->Rewrite(before);

  EXPECT_EQ(TypeId::kInt, after->GetTypeId());
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, NameExprOkLocalVar) {
  ParseProgram({
    {"A.java", "public class A { public A() { int i = 1; int a = i; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, NameExprOkLocalVarField) {
  ParseProgram({
    {"A.java", "public class A { public int a = 1; public A() { int a = this.a; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, NameExprLocalVarError) {
  ParseProgram({
    {"A.java", "public class A { public A() { boolean i = true; int a = i; } }"},
  });
  EXPECT_ERRS("UnassignableError(0:56)\n");
}

TEST_F(TypeCheckerTest, NameExprLocalVarErrorAssignSelf) {
  ParseProgram({
    {"A.java", "public class A { public int a = 1; public A() { int a = a; } }"},
  });
  EXPECT_ERRS("VariableInitializerSelfReferenceError(0:56)\n");
}

TEST_F(TypeCheckerTest, NameExprLocalVarErrorAssignSuppressed) {
  ParseProgram({
    {"A.java", "public class A { public A() { asdf i = true; int a = i; } }"},
  });
  EXPECT_ERRS("UnknownTypenameError(0:30-34)\n");
}

TEST_F(TypeCheckerTest, NameExprOkStaticField) {
  ParseProgram({
    {"A.java", "public class A { public A() { int i = foo.bar.B.i; } }"},
    {"B.java", "package foo.bar; public class B { public static int i; }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, NameExprStaticFieldError) {
  ParseProgram({
    {"A.java", "public class A { public A() { int i = foo.bar.B.i; } }"},
    {"B.java", "package foo.bar; public class B { protected static int i; }"},
  });
  EXPECT_ERRS("PermissionError(1:55)\n");
}

TEST_F(TypeCheckerTest, FieldDerefExprOk) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public int i; public int foo() { A a = new A(); return a.i; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, FieldDerefExprOnPrimitive) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public int i; public int foo() { return i.i; } }"},
  });
  EXPECT_ERRS("MemberAccessOnPrimitiveError(0:73)\n");
}

TEST_F(TypeCheckerTest, FieldDerefExprStaticNoType) {
  ParseProgram({
    {"A.java", "public class A { public int foo() { return B.i; } }"},
  });
  EXPECT_ERRS("UndefinedReferenceError(0:43)\n");
}

TEST_F(TypeCheckerTest, FieldDerefExprBadResolve) {
  ParseProgram({
    {"A.java", "package foo; public class A { public A() {} public int foo() { return B.i; } }"},
    {"B.java", "public class B { protected static int i; }"},
  });
  EXPECT_ERRS("UndefinedReferenceError(0:70)\n");
}

TEST_F(TypeCheckerTest, FieldDerefExprOnVoid) {
  ParseProgram({
    {"A.java", "public class A { public void foo() { int x = foo().bar; } }"},
  });
  EXPECT_ERRS("VoidInExprError(0:45-50)\n");
}

TEST_F(TypeCheckerTest, CallExprRecurseNameExprOk) {
  ParseProgram({
    {"A.java", "public class A { public A() { B.foo(); } }"},
    {"B.java", "public class B { public static void foo() {} }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, CallExprRecurseNameExprError) {
  ParseProgram({
    {"A.java", "public class A { public A() { a.foo(); } }"},
  });
  EXPECT_ERRS("UndefinedReferenceError(0:30)\n");
}

TEST_F(TypeCheckerTest, CallExprFieldDerefExprOk) {
  ParseProgram({
    {"A.java", "public class A { public void foo() {} public A() { foo(); } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, CallExprFieldDerefExprParamError) {
  ParseProgram({
    {"A.java", "public class A { public void foo(int i) {} public A() { foo(1, 2); } }"},
  });
  EXPECT_ERRS("UndefinedMethodError(0:56-59)\n");
}

TEST_F(TypeCheckerTest, CallExprOnVoid) {
  ParseProgram({
    {"A.java", "public class A { public void foo() { int x = foo().bar(); } }"},
  });
  EXPECT_ERRS("VoidInExprError(0:45-50)\n");
}

TEST_F(TypeCheckerTest, CallExprOnPrimitive) {
  ParseProgram({
    {"A.java", "public class A { public void foo() { 1.foo(); } }"},
  });
  EXPECT_ERRS("MemberAccessOnPrimitiveError(0:39-42)\n");
}

// TODO: NewArrayExpr

TEST_F(TypeCheckerTest, NewClassExpr) {
  ParseProgram({
      {"F.java", "public class F { public F() { F f=new F(); } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, NewClassExprArg) {
  ParseProgram({
      {"F.java", "public class F { public F(int i) { F f=new F(1); } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, NewClassExprBadConstructor) {
  ParseProgram({
      {"F.java", "public class F { public F(){ F f=new F(1); } }"}
  });
  EXPECT_ERRS("UndefinedMethodError(0:37)\n");
}

TEST_F(TypeCheckerTest, NewClassExprBadType) {
  ParseProgram({
      {"F.java", "public class F { public F() { F f=new A(); } }"}
  });
  EXPECT_ERRS("UnknownTypenameError(0:38)\n");
}

TEST_F(TypeCheckerTest, NewClassExprNewAbstractClass) {
  ParseProgram({
    {"A.java", "public abstract class A { public A() {} }"},
    {"B.java", "public class B { public B() { A a = new A(); } }"}
  });
  EXPECT_ERRS("NewAbstractClassError(1:40)\n");
}

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

TEST_F(TypeCheckerTest, ParenExprErrorTypeInside) {
  ParseProgram({
      {"A.java", "public class A { public A() { (Integer).intValue(); } }"}
  });
  EXPECT_ERRS("TypeInParensError(0:31-38)\n");
}

TEST_F(TypeCheckerTest, StringLitExpr) {
  ParseProgram({
      {"F.java", "public class F { public String f() { return \"Hi.\"; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, StringLitExprAddOtherThings) {
  ParseProgram({
      {"F.java", "public class F{ public String f() { return 1 + \"\" + 'a' + null; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, StringLitExprAddOtherThingsOneError) {
  ParseProgram({
      {"F.java", "public class F { public String f() { return null + 1 + \"\" + 'a' + null; } }"}
  });
  EXPECT_ERRS("TypeMismatchError(0:44-48)\n");
}

TEST_F(TypeCheckerTest, ThisLitExpr) {
  const auto insideType = TypeId{100, 0};

  sptr<const Expr> before = ParseExpr("this");

  auto typeChecker = (*typeChecker_.get())
    .InsideCompUnit(nullptr)
    .InsideTypeDecl(insideType)
    .InsideMemberDecl(false, TypeId::kVoid);

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
    .InsideMemberDecl(true, TypeId::kVoid);

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

TEST_F(TypeCheckerTest, UnaryExprOnVoid) {
  ParseProgram({
    {"A.java", "public class A { public void foo() { int x = -foo(); } }"},
  });
  EXPECT_ERRS("UnaryNonNumericError(0:45-51)\n");
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

TEST_F(TypeCheckerTest, ReturnStmt) {
  ParseProgram({
      {"F.java", "public class F { public int f() { return 1; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, ReturnStmtWrongType) {
  ParseProgram({
      {"F.java", "public class F { public int f() { return true; } }"}
  });
  EXPECT_ERRS("InvalidReturnError(0:34-40)\n");
}

TEST_F(TypeCheckerTest, ReturnStmtVoidMethodNonVoidReturn) {
  ParseProgram({
      {"F.java", "public class F { public void f() { return 1; } }"}
  });
  EXPECT_ERRS("ReturnInVoidMethodError(0:35-41)\n");
}

TEST_F(TypeCheckerTest, ReturnStmtNonVoidMethodVoidReturn) {
  ParseProgram({
      {"F.java", "public class F { public int f() { return; } }"}
  });
  EXPECT_ERRS("EmptyReturnInNonVoidMethodError(0:34-40)\n");
}

TEST_F(TypeCheckerTest, LocalDeclStmt) {
  ParseProgram({
      {"F.java", "public class F { public void f() { int x = 0; return; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, LocalDeclStmtBadTypeOneError) {
  ParseProgram({
      {"F.java", "public class F { public int f(){ A x = null; return x; } }"}
  });
  EXPECT_ERRS("UnknownTypenameError(0:33)\n");
}

TEST_F(TypeCheckerTest, LocalDeclStmtBadAssign) {
  ParseProgram({
      {"F.java", "public class F { public void f(){ char x = null; return; } }"}
  });
  EXPECT_ERRS("UnassignableError(0:43-47)\n");
}

TEST_F(TypeCheckerTest, LocalDeclStmtCreatesSymbol) {
  ParseProgram({
      {"F.java", "public class F { public int f(){ int x = 0; return x; } }"}
  });
  EXPECT_NO_ERRS();
}

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

// TODO.
TEST_F(TypeCheckerTest, DISABLED_FieldDeclThis) {
  sptr<const MemberDecl> before = ParseMemberDecl("int x = this;");
  auto typeChecker = (*typeChecker_.get())
    .InsideCompUnit(nullptr)
    .InsideTypeDecl(TypeId::kInt);
  auto after = typeChecker.Rewrite(before);

  EXPECT_NE(nullptr, after);
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerTest, FieldDeclStaticThis) {
  sptr<const MemberDecl> before = ParseMemberDecl("static int x = this;");
  auto typeChecker = (*typeChecker_.get())
    .InsideCompUnit(nullptr)
    .InsideTypeDecl(TypeId::kInt);
  auto after = typeChecker.Rewrite(before);

  EXPECT_EQ(nullptr, after);
  EXPECT_ERRS("ThisInStaticMemberError(0:15-19)\n");
}

// NOTE: InstanceOf, Cast, and BinExpr assign are tested in typechecker_hierarchy_test.cpp.

}  // namespace types
