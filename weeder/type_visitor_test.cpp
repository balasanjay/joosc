#include "weeder/type_visitor.h"
#include "weeder/weeder_test.h"

using base::ErrorList;
using base::Pos;
using lexer::K_INT;
using lexer::K_VOID;
using lexer::Token;
using parser::ArrayType;
using parser::MemberDecl;
using parser::PrimitiveType;
using parser::ReferenceType;
using parser::Stmt;
using parser::Type;
using parser::internal::Result;

namespace weeder {

class TypeVisitorTest : public WeederTest {
};

bool HasVoid(const Type*, Token* out);

TEST_F(TypeVisitorTest, HasVoidReferenceFalse) {
  Type* reference = new ReferenceType(nullptr);
  EXPECT_FALSE(HasVoid(reference, nullptr));

  Type* array = new ArrayType(reference);
  EXPECT_FALSE(HasVoid(array, nullptr));

  Type *array2 = new ArrayType(array);
  EXPECT_FALSE(HasVoid(array2, nullptr));

  delete array2;
}

TEST_F(TypeVisitorTest, HasVoidPrimitiveFalse) {
  Type* primitive = new PrimitiveType(Token(K_INT, Pos(0, 0)));
  EXPECT_FALSE(HasVoid(primitive, nullptr));

  Type* array = new ArrayType(primitive);
  EXPECT_FALSE(HasVoid(array, nullptr));

  Type *array2 = new ArrayType(array);
  EXPECT_FALSE(HasVoid(array2, nullptr));

  delete array2;
}

TEST_F(TypeVisitorTest, HasVoidTrue) {
  Token tok(K_VOID, Pos(0, 0));
  Token tokOut = tok;

  Type* voidType = new PrimitiveType(tok);
  EXPECT_TRUE(HasVoid(voidType, &tokOut));
  EXPECT_EQ(tok, tokOut);

  Type* array = new ArrayType(voidType);
  EXPECT_TRUE(HasVoid(array, &tokOut));
  EXPECT_EQ(tok, tokOut);

  Type* array2 = new ArrayType(array);
  EXPECT_TRUE(HasVoid(array, &tokOut));
  EXPECT_EQ(tok, tokOut);

  delete array2;
}

TEST_F(TypeVisitorTest, CastOk) {
  MakeParser("(int)3;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, CastNotOk) {
  MakeParser("(void)3;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:1-5)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, InstanceOfPrimitive) {
  MakeParser("a instanceof int;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidInstanceOfTypeError(0:2-12)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, InstanceOfArray) {
  MakeParser("a instanceof int[];");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}


TEST_F(TypeVisitorTest, NewClassOk) {
  MakeParser("new String();");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, NewClassVoid) {
  MakeParser("new void();");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:4-8)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, NewClassPrimitive) {
  MakeParser("new int();");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("NewNonReferenceTypeError(0:0-3)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, NewArrayOk) {
  MakeParser("new int[3];");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, NewArrayNotOk) {
  MakeParser("new void[3];");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:4-8)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, LocalDeclOk) {
  MakeParser("{int foo = 3;}");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
  EXPECT_EQ("", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, LocalDeclNotOk) {
  MakeParser("{void foo = 3;}");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:1-5)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, FieldDeclOk) {
  MakeParser("int foo = 3;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, FieldDeclNotOk) {
  MakeParser("void foo = 3;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:0-4)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ParamOk) {
  MakeParser("int main(){}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ParamNotOk) {
  MakeParser("int main(void a){}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidVoidTypeError(0:9-13)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForInitNotValid) {
  MakeParser("for(a + 1;;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:1)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForInitNotArrayAccess) {
  MakeParser("for(a[1];;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:1)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, ForInitNewClassAllowed) {
  MakeParser("for(new Foo(2);;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ForInitMethodCallAllowed) {
  MakeParser("for(a.b(2);;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ForInitAssignmentAllowed) {
  MakeParser("for(a = 1 + 2;;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(TypeVisitorTest, ForInitJustIdNotAllowed) {
  MakeParser("for(a;;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:1)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, DISABLED_ForInitParenedAssignmentDisallowed) {
  MakeParser("for((a = 1);;);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:1)\n", testing::PrintToString(errors));
}

TEST_F(TypeVisitorTest, BlockNotStmt) {
  MakeParser("{int a = 1; a = 2; a; b;}");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  TypeVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidTopLevelStatement(0:1)\nInvalidTopLevelStatement(0:1)\n", testing::PrintToString(errors));
}

} // namespace weeder

