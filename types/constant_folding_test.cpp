#include "types/types_test.h"

namespace types {

class ConstantFoldingTest: public TypesTest {
  void ReachabilityTest(string init, string expr) {
    ParseProgram({
      {"A.java", "public class A { public int a() { " + init + "while(" + expr + ") return 1; } }"}
    });
  }

public:
  void ShouldBeTrue(string init, string expr) {
    ReachabilityTest(init, expr);
    EXPECT_NO_ERRS();
  }

  void ShouldBeFalse(string init, string expr) {
    // Expecting unreachable while body and method needs a return.
    ReachabilityTest(init, expr);
    EXPECT_EQ(2, errors_.Size());
  }

  void ShouldBeUnknown(string init, string expr) {
    // Expecting method needs a return.
    ReachabilityTest(init, expr);
    EXPECT_EQ(1, errors_.Size());
  }
};

TEST_F(ConstantFoldingTest, ConstantTrueSanity) {
  ShouldBeTrue("", "true");
}

TEST_F(ConstantFoldingTest, ConstantFalseSanity) {
  ShouldBeFalse("", "false");
}

TEST_F(ConstantFoldingTest, ConstantUnknownSanity) {
  ShouldBeUnknown("boolean a = true;", "a");
}

} // namespace types
