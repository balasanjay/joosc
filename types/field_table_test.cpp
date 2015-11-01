#include "gtest/gtest.h"
#include "types/types_test.h"

namespace types {

class FieldTableTest : public TypesTest {};

TEST_F(FieldTableTest, Duplicate) {
  ParseProgram({
    {"A.java", "public class A { public int x; public int x; }"},
  });
  EXPECT_ERRS("x: [0:28,0:42,]\n");
}

TEST_F(FieldTableTest, DuplicateDiffTypes) {
  ParseProgram({
    {"A.java", "public class A { public int x; public boolean x; }"}
  });
  EXPECT_ERRS("x: [0:28,0:46,]\n");
}

TEST_F(FieldTableTest, DuplicateDiffContexts) {
  ParseProgram({
    {"A.java", "public class A { public int x; public static int x; }"}
  });
  EXPECT_ERRS("x: [0:28,0:49,]\n");
}

TEST_F(FieldTableTest, InheritedField) {
  ParseProgram({
    {"A.java", "public class A { public int x; public A() {} }"},
    {"B.java", "public class B extends A { public int f() { return x; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, InheritedStaticField) {
  ParseProgram({
    {"A.java", "public class A { public static int x; public A() {} }"},
    {"B.java", "public class B extends A { public int f() { return B.x; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, Uninitialized) {
  ParseProgram({
    {"A.java", "public class A { public int x; }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, Initialized) {
  ParseProgram({
    {"A.java", "public class A { public int x = 0; }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, ReferencingAnother) {
  ParseProgram({
    {"A.java", "public class A { public int x; public int y = x; }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, ReferencingMemberOtherClass) {
  ParseProgram({
    {"A.java", "public class A { public int x; public A () {} }"},
    {"B.java", "public class B { public int f() { return new A().x; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, ReferencingMemberSameClass) {
  ParseProgram({
    {"A.java", "public class A { public int x; public int f() { return x; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, ReferencingStaticOtherClass) {
  ParseProgram({
    {"A.java", "public class A { public static int x; }"},
    {"B.java", "public class B { public int f(){ return A.x; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, ReferencingStaticSameClass) {
  ParseProgram({
    {"A.java", "public class A { public static int x; public int f() { return A.x; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, ReferencingStaticSameClassNoQualifier) {
  ParseProgram({
    {"A.java", "public class A { public static int x; public int f() { return x; } }"},
  });
  EXPECT_ERRS("StaticFieldOnInstanceError(0:62)\n");
}

TEST_F(FieldTableTest, ReferencingNonExistantField) {
  ParseProgram({
    {"A.java", "public class A { public A() {} }"},
    {"B.java", "public class B { public int f() { return new A().foo; }}"}
  });
  EXPECT_ERRS("UndefinedReferenceError(1:49-52)\n");
}

TEST_F(FieldTableTest, ReferencingInstanceFromStaticOtherClass) {
  ParseProgram({
    {"A.java", "public class A { public int foo; public A() {} }"},
    {"B.java", "public class B { public int f() { return A.foo; } }"}
  });
  EXPECT_ERRS("InstanceFieldOnStaticError(1:43-46)\n");
}

TEST_F(FieldTableTest, ReferencingInstanceFromStaticSameClass) {
  ParseProgram({
    {"A.java", "public class A { public int x; public static int f() { return A.x; } }"}
  });
  EXPECT_ERRS("InstanceFieldOnStaticError(0:64)\n");
}

TEST_F(FieldTableTest, ReferencingStaticFromInstanceOtherClass) {
  ParseProgram({
    {"A.java", "public class A { public static int x; public A() {} }"},
    {"B.java", "public class B { public int f() { return new A().x; } }"}
  });
  EXPECT_ERRS("StaticFieldOnInstanceError(1:49)\n");
}

TEST_F(FieldTableTest, ReferencingStaticFromInstanceSameClass) {
  ParseProgram({
    {"A.java", "public class A { public static int x;  public int f(){ return x; } }"},
  });
  EXPECT_ERRS("StaticFieldOnInstanceError(0:62)\n");
}

TEST_F(FieldTableTest, ProtectedAccessInChild) {
  ParseProgram({
    {"A.java", "public class A { protected int x; public A() {} }"},
    {"B.java", "public class B extends A { public int f() { return x; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, ProtectedAccessPackage) {
  ParseProgram({
    {"p/A.java", "package p; public class A { protected static int x; }"},
    {"p/B.java", "package p; public class B { public int f() { return A.x; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(FieldTableTest, ProtectedAccessOutsidePackage) {
  ParseProgram({
    {"p/A.java", "package p; public class A { protected static int x; }"},
    {"p2/B.java", "package p2; import p.A; public class B { public int f() { return A.x; } }"}
  });
  EXPECT_ERRS("PermissionError(0:49)\n");
}

// TODO: Test only using fields defined above.
// TODO: Test that in field initializer having both own field and external type will resolve to currently initializing field and fail.

} // namespace types

