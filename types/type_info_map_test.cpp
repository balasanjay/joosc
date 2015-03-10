#include "types/types_test.h"

namespace types {

class TypeInfoMapTest : public TypesTest {};

TEST_F(TypeInfoMapTest, CyclicGraph) {
  ParseProgram({
    {"Foo.java", "public class Foo extends Bar {}"},
    {"Bar.java", "public class Bar extends Baz {}"},
    {"Baz.java", "public class Baz extends Foo {}"},
  });
  EXPECT_ERRS("ExtendsCycleError{Bar->Baz,Baz->Foo,Foo->Bar,}\n");
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
