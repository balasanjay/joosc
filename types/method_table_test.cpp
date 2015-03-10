#include "third_party/gtest/gtest.h"
#include "types/types_test.h"

using ast::Program;

#define EXPECT_ERRS(msg) EXPECT_EQ(msg, testing::PrintToString(errors_))
#define EXPECT_NO_ERRS() EXPECT_EQ(0, errors_.Size())
#define PRINT_ERRS() errors_.PrintTo(&std::cerr, base::OutputOptions::kUserOutput, fs_.get())

namespace types {

class MethodTableTest: public ::testing::Test {
protected:
  sptr<const Program> ParseProgram(const vector<pair<string, string>>& file_contents) {
    base::FileSet* fs;
    sptr<const Program> program = ParseProgramWithStdlib(&fs, file_contents, &errors_);
    fs_.reset(fs);
    return program;
  }

  base::ErrorList errors_;
  uptr<base::FileSet> fs_;
};

TEST_F(MethodTableTest, BadConstructorError) {
  ParseProgram({
    {"A.java", "public final class A { public B() {} }"},
  });
  EXPECT_ERRS("ConstructorNameError(0:30)\n");
}

TEST_F(MethodTableTest, DuplicateError) {
  ParseProgram({
    {"A.java", "public class A { public void foo() {} public void foo() {} }"},
  });
  EXPECT_ERRS("foo: [0:29-32,0:50-53,]\n");
}

TEST_F(MethodTableTest, DuplicateDifferentReturnTypeError) {
  ParseProgram({
    {"A.java", "public class A { public void foo() {} public int foo() { return 1; } }"},
  });
  EXPECT_ERRS("foo: [0:29-32,0:49-52,]\n");
}

TEST_F(MethodTableTest, FinalAncestorErrorNoExtraErrors) {
  ParseProgram({
    {"A.java", "public final class A { public A() {} }"},
    {"B.java", "public class B extends A { public B() {} }"},
    {"C.java", "public class C extends B {}"},
  });
  EXPECT_ERRS("ParentFinalError: [1:13,0:19]\n");
}

TEST_F(MethodTableTest, SimpleInheritNoErrors) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public void foo() {} }"},
    {"B.java", "public class B extends A { public B() {} public void bar() {} }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(MethodTableTest, DiamondInheritNoErrors) {
  ParseProgram({
    {"A.java", "public interface A { public void foo(); }"},
    {"B.java", "public interface B extends A {}"},
    {"C.java", "public interface C extends A {}"},
    {"D.java", "public interface D extends B, C {}"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(MethodTableTest, DifferentReturnTypeError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public void foo() {} }"},
    {"B.java", "public class B extends A { public int foo() { return 1; } }"},
  });
  EXPECT_ERRS("DifferingReturnTypeError: [1:38-41,0:43-46]\n");
}

TEST_F(MethodTableTest, OverloadStaticError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public static void foo() {} }"},
    {"B.java", "public class B extends A { public void foo() {} }"},
  });
  EXPECT_ERRS("StaticMethodOverrideError: [1:39-42,0:50-53]\n");
}

TEST_F(MethodTableTest, OverloadUsingStaticError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public void foo() {} }"},
    {"B.java", "public class B extends A { public static void foo() {} }"},
  });
  EXPECT_ERRS("StaticMethodOverrideError: [1:46-49,0:43-46]\n");
}

TEST_F(MethodTableTest, OverloadStaticUsingStaticError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public static void foo() {} }"},
    {"B.java", "public class B extends A { public static void foo() {} }"},
  });
  EXPECT_ERRS("StaticMethodOverrideError: [1:46-49,0:50-53]\n");
}

TEST_F(MethodTableTest, LowerVisibilityError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public void foo() {} }"},
    {"B.java", "public class B extends A { protected void foo() {} }"},
  });
  EXPECT_ERRS("LowerVisibilityError: [1:42-45,0:43-46]\n");
}

TEST_F(MethodTableTest, FinalOverrideError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public final void foo() {} }"},
    {"B.java", "public class B extends A { public void foo() {} }"},
  });
  EXPECT_ERRS("OverrideFinalMethodError: [1:39-42,0:49-52]\n");
}

TEST_F(MethodTableTest, ParentClassNoEmptyConstructorError) {
  ParseProgram({
    {"A.java", "public class A { }"},
    {"B.java", "public class B extends A { }"},
  });
  EXPECT_ERRS("ParentClassEmptyConstructorError: [0:13,1:13]\n");
}

TEST_F(MethodTableTest, NotAbstractClassError) {
  ParseProgram({
    {"A.java", "public abstract class A { public A() {} public abstract void foo(); }"},
    {"B.java", "public class B extends A { }"},
  });
  EXPECT_ERRS("NeedAbstractClassError: [1:13]\n");
}

TEST_F(MethodTableTest, ResolveCallUndefinedMethodError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public void foo() { bar(); } }"},
  });
  EXPECT_ERRS("UndefinedMethodError(0:51-54)\n");
}

TEST_F(MethodTableTest, ResolveCallInstanceAsStaticError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public static void foo() { A.bar(); } public void bar() {} }"},
  });
  EXPECT_ERRS("StaticMethodOnInstanceError(0:60-63)\n");
}

TEST_F(MethodTableTest, ResolveCallStaticAsInstanceError) {
  ParseProgram({
    {"A.java", "public class A { public A() {} public static void foo() {} public void bar() { foo(); } }"},
  });
  EXPECT_ERRS("InstanceMethodOnStaticError(0:79-82)\n");
}

TEST_F(MethodTableTest, ResolveCallInaccessibleError) {
  ParseProgram({
    {"A.java", "package foo; public class A { public A() {} protected void foo() {} }"},
    {"B.java", "package baz; import foo.A; public class B { public void bar() { A a = new A(); a.foo(); } }"},
  });
  EXPECT_ERRS("PermissionError: [1:81-84,0:59-62]\n");
}

} // namespace types
