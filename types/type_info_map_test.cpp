#include "types/types_test.h"

namespace types {

class TypeInfoMapTest : public TypesTest {};

namespace {
  pair<string, string> DiamondProblemHelper(char c, int i) {
    stringstream ss;
    ss << c << i << ".java";
    const string file_name = ss.str();
    ss.str("");
    switch (c) {
      case 'C': {
        if (i == 0) {
          return {file_name, "package c0; public interface C0 {}"};
        }
        ss << "package c" << i << "; public interface C" << i << " extends l" << i - 1 << ".L" << i - 1 << ", r" << i - 1 << ".R" << i - 1 << " {}";
        return {file_name, ss.str()};
      }
      case 'L': {
        ss << "package l" << i << "; public interface L" << i << " extends c" << i << ".C" << i << "{}";
        return {file_name, ss.str()};
      }
      case 'R': {
        ss << "package r" << i << "; public interface R" << i << " extends c" << i << ".C" << i << "{}";
        return {file_name, ss.str()};
      }
      default:
        throw;
    }
  }
} // namespace

TEST_F(TypeInfoMapTest, CyclicGraph) {
  ParseProgram({
    {"Foo.java", "public class Foo extends Bar {}"},
    {"Bar.java", "public class Bar extends Baz {}"},
    {"Baz.java", "public class Baz extends Foo {}"},
  });
  EXPECT_ERRS("ExtendsCycleError{Bar->Baz,Baz->Foo,Foo->Bar,}\n");
}

TEST_F(TypeInfoMapTest, SelfCyclicGraph) {
  ParseProgram({
    {"Foo.java", "public class Foo extends Foo {}"},
  });
  EXPECT_ERRS("ExtendsCycleError{Foo->Foo,}\n");
}

TEST_F(TypeInfoMapTest, DiamondProblemNotBad) {
  vector<pair<string, string>> test_files;
  for (int i = 0; i < 100; ++i) {
    test_files.push_back(DiamondProblemHelper('C', i));
    test_files.push_back(DiamondProblemHelper('L', i));
    test_files.push_back(DiamondProblemHelper('R', i));
  }
  test_files.push_back({"C100.java", "public class C100 implements l99.L99, r99.R99 {}"});
  ParseProgram(test_files);
  EXPECT_NO_ERRS();
  PRINT_ERRS();
}

TEST_F(TypeInfoMapTest, InterfaceExtendingRepeatedInterface) {
  ParseProgram({
    {"Foo.java", "public interface Foo {}"},
    {"Bar.java", "public interface Bar extends Foo, Foo, Foo, Foo {}"},
  });
  EXPECT_ERRS("DuplicateInheritanceError(0:17-20)\n");
}

TEST_F(TypeInfoMapTest, InterfaceExtendingClass) {
  ParseProgram({
    {"Foo.java", "public class Foo {}"},
    {"Bar.java", "public interface Bar extends Foo {}"},
  });
  EXPECT_ERRS("InterfaceExtendsClassError(0:17-20)\n");
}

TEST_F(TypeInfoMapTest, ClassExtendingInterface) {
  ParseProgram({
    {"Foo.java", "public interface Foo {}"},
    {"Bar.java", "public class Bar extends Foo {}"},
  });
  EXPECT_ERRS("ClassExtendInterfaceError(0:13-16)\n");
}

TEST_F(TypeInfoMapTest, ClassImplementingClass) {
  ParseProgram({
    {"Foo.java", "public class Foo {}"},
    {"Bar.java", "public class Bar implements Foo {}"},
  });
  EXPECT_ERRS("ClassImplementsClassError(0:13-16)\n");
}

TEST_F(TypeInfoMapTest, ClassImplementingRepeatedInterface) {
  ParseProgram({
    {"Foo.java", "public interface Foo {}"},
    {"Bar.java", "public class Bar implements Foo, Foo, Foo, Foo {}"},
  });
  EXPECT_ERRS("DuplicateInheritanceError(0:13-16)\n");
}

} // namespace types
