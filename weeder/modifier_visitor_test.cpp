#include "weeder/modifier_visitor.h"

#include "weeder/weeder_test.h"

using ast::MemberDecl;
using ast::TypeDecl;
using base::ErrorList;
using parser::internal::Result;

namespace weeder {

class ModifierVisitorTest : public WeederTest {};

TEST_F(ModifierVisitorTest, ClassConstructorDeclConflicting) {
  MakeParser("public protected x(){};");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  string expected =
      "ConflictingAccessModError(0:0-6)\n"
      "ConflictingAccessModError(0:7-16)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassConstructorDeclDisallowed) {
  MakeParser("abstract static final native public x(){}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  string expected =
      "ClassConstructorModifierError(0:0-8)\n"
      "ClassConstructorModifierError(0:9-15)\n"
      "ClassConstructorModifierError(0:16-21)\n"
      "ClassConstructorModifierError(0:22-28)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassConstructorDeclInvalidEmpty) {
  MakeParser("public x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassConstructorEmptyError(0:7)\n",
            testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassConstructorOk) {
  MakeParser("public x() { int x = 1; }");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(ModifierVisitorTest, ClassFieldDeclConflicting) {
  MakeParser("public protected int x = 1;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  string expected =
      "ConflictingAccessModError(0:0-6)\n"
      "ConflictingAccessModError(0:7-16)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassFieldDeclDisallowed) {
  MakeParser("abstract final native public int x = 1;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  string expected =
      "ClassFieldModifierError(0:0-8)\n"
      "ClassFieldModifierError(0:9-14)\n"
      "ClassFieldModifierError(0:15-21)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassFieldOk) {
  MakeParser("public static int x = 1;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(ModifierVisitorTest, ClassMethodDeclConflicting) {
  MakeParser("public protected int x() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  string expected =
      "ConflictingAccessModError(0:0-6)\n"
      "ConflictingAccessModError(0:7-16)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodDeclInvalidEmpty) {
  MakeParser("public int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodEmptyError(0:11)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodDeclInvalidBody) {
  MakeParser("public abstract int x() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodNotEmptyError(0:20)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodAbstractStatic) {
  MakeParser("abstract static public int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodAbstractModifierError(0:9-15)\n",
            testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodAbstractFinal) {
  MakeParser("abstract final public int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodAbstractModifierError(0:9-14)\n",
            testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodStaticFinal) {
  MakeParser("static final public int x() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodStaticFinalError(0:7-12)\n",
            testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodNativeNotStatic) {
  MakeParser("native public int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodNativeNotStaticError(0:0-6)\n",
            testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodOk) {
  MakeParser("public static int main() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(ModifierVisitorTest, InterfaceConstructorDeclFail) {
  MakeParser("Foo(){}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InterfaceConstructorError(0:0-3)\n",
            testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, InterfaceFieldDeclFail) {
  MakeParser("int x = 3;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InterfaceFieldError(0:4)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, InterfaceMethodDisallowed) {
  MakeParser("protected static final native public int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  string expected =
      "InterfaceMethodModifierError(0:0-9)\n"
      "InterfaceMethodModifierError(0:10-16)\n"
      "InterfaceMethodModifierError(0:17-22)\n"
      "InterfaceMethodModifierError(0:23-29)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, InterfaceMethodDeclInvalidBody) {
  MakeParser("public int x() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InterfaceMethodImplError(0:11)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, InterfaceMethodOk) {
  MakeParser("public abstract int main();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(ModifierVisitorTest, ClassBadModifiers) {
  MakeParser("protected static native public class Foo{}");
  Result<TypeDecl> decl;
  ASSERT_FALSE(parser_->ParseTypeDecl(&decl).Failed());

  ErrorList errors;
  ModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  string expected =
      "ClassModifierError(0:0-9)\n"
      "ClassModifierError(0:10-16)\n"
      "ClassModifierError(0:17-23)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassBadAbstractFinal) {
  MakeParser("public abstract final class Foo{}");
  Result<TypeDecl> decl;
  ASSERT_FALSE(parser_->ParseTypeDecl(&decl).Failed());

  ErrorList errors;
  ModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("AbstractFinalClass(0:28-31)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassOk) {
  MakeParser("public class Foo{}");
  Result<TypeDecl> decl;
  ASSERT_FALSE(parser_->ParseTypeDecl(&decl).Failed());

  ErrorList errors;
  ModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(ModifierVisitorTest, InterfaceBadModifiers) {
  MakeParser("protected static final native public interface Foo{}");
  Result<TypeDecl> decl;
  ASSERT_FALSE(parser_->ParseTypeDecl(&decl).Failed());

  ErrorList errors;
  ModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  string expected =
      "InterfaceModifierError(0:0-9)\n"
      "InterfaceModifierError(0:10-16)\n"
      "InterfaceModifierError(0:17-22)\n"
      "InterfaceModifierError(0:23-29)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, InterfaceOk) {
  MakeParser("public interface Foo{}");
  Result<TypeDecl> decl;
  ASSERT_FALSE(parser_->ParseTypeDecl(&decl).Failed());

  ErrorList errors;
  ModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(ModifierVisitorTest, RecursionInterfaceOk) {
  MakeParser("public interface Foo { public void foo(){} }");
  Result<TypeDecl> decl;
  ASSERT_FALSE(parser_->ParseTypeDecl(&decl).Failed());

  ErrorList errors;
  ModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InterfaceMethodImplError(0:35-38)\n",
            testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, RecursionClassOk) {
  MakeParser("public class Foo { public void foo(); }");
  Result<TypeDecl> decl;
  ASSERT_FALSE(parser_->ParseTypeDecl(&decl).Failed());

  ErrorList errors;
  ModifierVisitor visitor(&errors);
  visitor.Visit(decl.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodEmptyError(0:31-34)\n", testing::PrintToString(errors));
}

}  // namespace weeder
