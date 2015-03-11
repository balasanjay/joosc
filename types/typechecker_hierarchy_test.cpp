#include "types/typechecker.h"

#include "base/fileset.h"
#include "types/types_test.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/parser.h"
#include "types/typeset.h"
#include "third_party/gtest/gtest.h"

using namespace ast;

using base::FileSet;
using base::ErrorList;
using base::Pos;
using base::PosRange;
using lexer::Token;
using ast::Program;

#define EXPECT_ERRS(msg) EXPECT_EQ(msg, testing::PrintToString(errors_))
#define EXPECT_NO_ERRS() EXPECT_EQ(0, errors_.Size())

namespace types {

class TypeCheckerHierarchyTest : public TypesTest {
protected:
  sptr<const Program> TestSimpleHierarchy(const vector<pair<string, string>>& extra_file_contents) {
    vector<pair<string, string>> file_contents = {
      {"A.java", "public class A { public A() {} }"},
      {"B.java", "public class B extends A { public B() {} }"},
      {"C.java", "public class C extends B { public C() {} }"},
      {"D.java", "public class D extends C { public D() {} }"},
      {"F.java", "public class F { public F() {} }"},
    };
    for (auto f : extra_file_contents) {
      file_contents.push_back(f);
    }
    return ParseProgram(file_contents);
  }

  sptr<const Program> TestInterfaceDag(const vector<pair<string, string>>& extra_file_contents) {
    vector<pair<string, string>> file_contents = {
      {"IA.java", "public interface IA {}"},
      {"IB.java", "public interface IB extends IA {}"},
      {"IC.java", "public interface IC {}"},
      {"ID.java", "public interface ID extends IB, IC {}"},
      {"A.java", "public class A implements IA { public A() {} }"},
      {"B.java", "public class B extends A implements IC { public B() {} }"},
      {"C.java", "public class C implements ID { public C() {} }"},
    };
    for (auto f : extra_file_contents) {
      file_contents.push_back(f);
    }
    return ParseProgram(file_contents);
  }
};

TEST_F(TypeCheckerHierarchyTest, InstanceOfAncestor) {
  TestSimpleHierarchy({
      {"Main.java", "public class Main { public boolean f() { return new D() instanceof A; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, InstanceOfDescendant) {
  TestSimpleHierarchy({
      {"Main.java", "public class Main { public boolean f() { return new A() instanceof D; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, InstanceOfSelf) {
  TestSimpleHierarchy({
      {"Main.java", "public class Main { public boolean f() { return new F() instanceof F; } }"}
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, InstanceOfUnrelated) {
  TestSimpleHierarchy({
      {"Main.java", "public class Main { public boolean f() { return new F() instanceof A; } }"}
  });
  EXPECT_ERRS("IncompatibleInstanceOfError(5:48-68)\n");
}

TEST_F(TypeCheckerHierarchyTest, InstanceOfPrimitive) {
  ParseProgram({
      {"F.java", "public class F { public boolean f() { return 1 instanceof int; } }"}
  });
  EXPECT_ERRS("InvalidInstanceOfTypeError(0:47-57)\n");
}

TEST_F(TypeCheckerHierarchyTest, IsCastablePrimitives) {
  TypeChecker typeChecker(nullptr);
  auto numTids = {TypeId::kInt, TypeId::kChar, TypeId::kShort, TypeId::kByte};
  for (TypeId tidA : numTids) {
    for (TypeId tidB : numTids) {
      EXPECT_TRUE(typeChecker.IsCastable(tidA, tidB));
    }
  }
  EXPECT_TRUE(typeChecker.IsCastable(TypeId::kBool, TypeId::kBool));
}

TEST_F(TypeCheckerHierarchyTest, CastToAncestor) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { A a = (A)new D(); return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, CastToDescendant) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { D a = (D)new A(); return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, CastToUnrelated) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { F a = (F)new A(); return; } }"},
  });
  EXPECT_ERRS("IncompatibleCastError(5:46-56)\n");
}

TEST_F(TypeCheckerHierarchyTest, CastFromObject) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { F a = (F)(Object)new A(); return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, CastArrayRelatedBases) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { D[] a = (D[])new A[1]; return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, CastArrayOfTypeToType) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { D a = (D)new D[1]; return; } }"},
  });
  EXPECT_ERRS("IncompatibleCastError(5:46-57)\n");
}

// TODO: Cast array to object and vice versa.

TEST_F(TypeCheckerHierarchyTest, CastRefToPrimitive) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { int a = (int)new A(); return; } }"},
  });
  EXPECT_ERRS("IncompatibleCastError(5:48-60)\n");
}

TEST_F(TypeCheckerHierarchyTest, CastPrimitiveToRef) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { A a = (A)1; return; } }"},
  });
  EXPECT_ERRS("IncompatibleCastError(5:46-50)\n");
}

TEST_F(TypeCheckerHierarchyTest, CastInterfaceToClass) {
  TestInterfaceDag({
    {"Main.java", "public class Main { public void foo() { C a = (C)(IA)new A(); return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, CastNonDescendantInterfaces) {
  TestInterfaceDag({
    {"Main.java", "public class Main { public void foo() { IC a = (IC)(IB)new D(); return; } }"},
  });
  EXPECT_ERRS("UnknownTypenameError(7:59)\n");
}

TEST_F(TypeCheckerHierarchyTest, AssignDescendant) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { A a = new D(); return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, AssignAncestor) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { D a = new A(); return; } }"},
  });
  EXPECT_ERRS("UnassignableError(5:46-53)\n");
}

TEST_F(TypeCheckerHierarchyTest, AssignToObject) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { Object a = new A(); return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, AssignArrayToAncestorArray) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { A[] a = new D[1]; return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, AssignArrayToDescendantArray) {
  TestSimpleHierarchy({
    {"Main.java", "public class Main { public void foo() { D[] a = new A[1]; return; } }"},
  });
  EXPECT_ERRS("UnassignableError(5:48-56)\n");
}

TEST_F(TypeCheckerHierarchyTest, AssignClassToInterface) {
  TestInterfaceDag({
    {"Main.java", "public class Main { public void foo() { IA a = new A(); return; } }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeCheckerHierarchyTest, AssignInterfaceToInterface) {
  TestInterfaceDag({
    {"Main.java", "public class Main { public void foo() { IA a = (ID)new C(); return; } }"},
  });
  EXPECT_NO_ERRS();
}

}  // namespace types
