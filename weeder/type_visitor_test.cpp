#include "weeder/type_visitor.h"

#include "weeder/weeder_test.h"

using ast::ArrayType;
using ast::MemberDecl;
using ast::PrimitiveType;
using ast::QualifiedName;
using ast::ReferenceType;
using ast::Expr;
using ast::Stmt;
using ast::Type;
using base::ErrorList;
using base::Pos;
using lexer::K_INT;
using lexer::K_VOID;
using lexer::LBRACK;
using lexer::RBRACK;
using lexer::Token;
using parser::internal::Result;

namespace weeder {

class TypeVisitorTest : public WeederTest {};

bool HasVoid(const Type&, Token* out);

TEST_F(TypeVisitorTest, HasVoidReferenceFalse) {
  sptr<Type> reference = make_shared<ReferenceType>(QualifiedName());
  EXPECT_FALSE(HasVoid(*reference, nullptr));

  sptr<Type> array = make_shared<ArrayType>(reference, Token(LBRACK, Pos(-1, -1)), Token(RBRACK, Pos(-1, -1)));
  EXPECT_FALSE(HasVoid(*array, nullptr));

  sptr<Type> array2 = make_shared<ArrayType>(array, Token(LBRACK, Pos(-1, -1)), Token(RBRACK, Pos(-1, -1)));
  EXPECT_FALSE(HasVoid(*array2, nullptr));
}

TEST_F(TypeVisitorTest, HasVoidPrimitiveFalse) {
  sptr<Type> primitive = make_shared<PrimitiveType>(Token(K_INT, Pos(0, 0)));
  EXPECT_FALSE(HasVoid(*primitive, nullptr));

  sptr<Type> array = make_shared<ArrayType>(primitive, Token(LBRACK, Pos(-1, -1)), Token(RBRACK, Pos(-1, -1)));
  EXPECT_FALSE(HasVoid(*array, nullptr));

  sptr<Type> array2 = make_shared<ArrayType>(array, Token(LBRACK, Pos(-1, -1)), Token(RBRACK, Pos(-1, -1)));
  EXPECT_FALSE(HasVoid(*array2, nullptr));
}

TEST_F(TypeVisitorTest, HasVoidTrue) {
  Token tok(K_VOID, Pos(0, 0));
  Token tokOut = tok;

  sptr<Type> voidType = make_shared<PrimitiveType>(tok);
  EXPECT_TRUE(HasVoid(*voidType, &tokOut));
  EXPECT_EQ(tok, tokOut);

  sptr<Type> array = make_shared<ArrayType>(voidType, Token(LBRACK, Pos(-1, -1)), Token(RBRACK, Pos(-1, -1)));
  EXPECT_TRUE(HasVoid(*array, &tokOut));
  EXPECT_EQ(tok, tokOut);

  sptr<Type> array2 = make_shared<ArrayType>(array, Token(LBRACK, Pos(-1, -1)), Token(RBRACK, Pos(-1, -1)));
  EXPECT_TRUE(HasVoid(*array, &tokOut));
  EXPECT_EQ(tok, tokOut);
}

TEST_F(TypeVisitorTest, CastOk) {
  MakeParser("(int)3;");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, CastNotOk) {
  MakeParser("(void)3;");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:1-5)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, InstanceOfPrimitive) {
  MakeParser("a instanceof int;");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidInstanceOfTypeError(0:2-12)\n",
            testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, InstanceOfArray) {
  MakeParser("a instanceof int[];");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, NewClassOk) {
  MakeParser("new String();");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, NewClassVoid) {
  MakeParser("new void();");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:4-8)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, NewClassPrimitive) {
  MakeParser("new int();");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("NewNonReferenceTypeError(0:0-3)\n",
            testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, NewArrayOk) {
  MakeParser("new int[3];");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, NewArrayNotOk) {
  MakeParser("new void[3];");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:4-8)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, LocalDeclOk) {
  MakeParser("{int foo = 3;}");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_FALSE(errors.IsFatal());
  EXPECT_EQ("", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, LocalDeclNotOk) {
  MakeParser("{void foo = 3;}");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:1-5)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, FieldDeclOk) {
  MakeParser("int foo = 3;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(decl.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, FieldDeclNotOk) {
  MakeParser("void foo = 3;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:0-4)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ParamOk) {
  MakeParser("int main(){}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(decl.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ParamNotOk) {
  MakeParser("int main(void a){}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:9-13)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForInitNotValid) {
  MakeParser("for(a + 1;;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:4-9)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForInitNotArrayAccess) {
  MakeParser("for(a[1];;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:4-8)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForInitNewClassAllowed) {
  MakeParser("for(new Foo(2);;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ForInitMethodCallAllowed) {
  MakeParser("for(a.b(2);;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ForInitAssignmentAllowed) {
  MakeParser("for(a = 1 + 2;;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ForInitJustIdNotAllowed) {
  MakeParser("for(a;;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:4)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForInitAssignmentInParensDisallowed) {
  MakeParser("for((a = 1);;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:4-11)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForInitUpdateExprWorks) {
  MakeParser("for(;;i = i+1);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ForInitUpdateExprNotTopLevel) {
  MakeParser("for(;;i+1);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:6-9)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForBodyNotTopLevel) {
  MakeParser("for(;;) true;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:8-12)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, IfBodyNotTopLevel) {
  MakeParser("if(true) true;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:9-13)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, BlockNotStmt) {
  MakeParser("{int a = 1; a = 2; a; b;}");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(stmt.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:19)\nInvalidTopLevelStatement(0:22)\n",
            testing::PrintToString(errors));
}

}  // namespace weeder
