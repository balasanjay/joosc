#include "weeder/modifier_visitor.h"
#include "weeder/weeder_test.h"

using base::ErrorList;
using parser::MemberDecl;
using parser::internal::Result;

namespace weeder {

class ModifierVisitorTest : public WeederTest {
};

TEST_F(ModifierVisitorTest, ClassFieldDeclConflicting) {
  MakeParser("public protected int x = 1;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  string expected =
"ConflictingAccessModError(0:0-6)\n"
"ConflictingAccessModError(0:7-16)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassFieldDeclDisallowed) {
  MakeParser("abstract final native int x = 1;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

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
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(ModifierVisitorTest, ClassMethodDeclConflicting) {
  MakeParser("public protected int x() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

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
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodEmptyError(0:11)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodDeclInvalidBody) {
  MakeParser("public abstract int x() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodNotEmptyError(0:20)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodAbstractStatic) {
  MakeParser("abstract static int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodAbstractModifierError(0:9-15)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodAbstractFinal) {
  MakeParser("abstract final int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodAbstractModifierError(0:9-14)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodStaticFinal) {
  MakeParser("static final int x() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodStaticFinalError(0:7-12)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodNativeNotStatic) {
  MakeParser("native int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("ClassMethodNativeNotStaticError(0:0-6)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, ClassMethodOk) {
  MakeParser("public static int main() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  ClassModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(ModifierVisitorTest, InterfaceFieldDeclFail) {
  MakeParser("int x = 3;");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InterfaceFieldError(0:4)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, InterfaceMethodDisallowed) {
  MakeParser("protected static final native int x();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  string expected =
"InterfaceMethodModifierError(0:0-9)\n"
"InterfaceMethodModifierError(0:10-16)\n"
"InterfaceMethodModifierError(0:17-22)\n"
"InterfaceMethodModifierError(0:23-29)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, InterfaceMethodDeclInvalidBody) {
  MakeParser("int x() {}");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InterfaceMethodImplError(0:4)\n", testing::PrintToString(errors));
}

TEST_F(ModifierVisitorTest, InterfaceMethodOk) {
  MakeParser("public abstract int main();");
  Result<MemberDecl> decl;
  ASSERT_FALSE(parser_->ParseMemberDecl(&decl).Failed());

  ErrorList errors;
  InterfaceModifierVisitor visitor(fs_.get(), &errors);
  decl.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

} // namespace weeder
