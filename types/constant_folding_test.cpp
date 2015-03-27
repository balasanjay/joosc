#include "types/types_test.h"

namespace types {

class ConstantFoldingTest: public TypesTest {
  // Relies on reachability to check whether the constant
  // was folded at compile-time.
  void ReachabilityTest(string init, string expr) {
    ParseProgram({
      {"A.java", "public class A { public int a() { " + init + "while(" + expr + ") return 1; } }"}
    });
  }

public:
  void ShouldBeTrue(string init, string expr) {
    ReachabilityTest(init, expr);
    EXPECT_NO_ERRS();
    if (errors_.Size() > 0) {
      PRINT_ERRS();
    }
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

TEST_F(ConstantFoldingTest, ConstantIntExpr) {
  ShouldBeTrue("", "(1 - 3)*4 / 2 == -4");
}

TEST_F(ConstantFoldingTest, ConstantBoolExpr) {
  ShouldBeFalse("", "false || (false || false) || true && false");
}

TEST_F(ConstantFoldingTest, NotConstantVariable) {
  ShouldBeUnknown("boolean a = false;", "false || (false || false) || !a && false");
}

TEST_F(ConstantFoldingTest, CastBoolToBool) {
  ShouldBeTrue("", "(boolean)true");
}


} // namespace types
