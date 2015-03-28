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

TEST_F(ConstantFoldingTest, IntNarrowing) {
  stringstream ss;
  ss << "(short)"
     << ((1 << 17) + (1 << 9) + 1)
     << " + (byte)"
     << ((1 << 9) + 1)
     << " == "
     << ((1 << 9) + 2);
  ShouldBeTrue("", ss.str());
}

TEST_F(ConstantFoldingTest, OverflowTest) {
  stringstream ss;
  i64 i32max = (1 << 31) - 1;
  i64 i32min = -(1 << 31);
  ss << i32max
     << " + 1 == "
     << i32min;
  ShouldBeTrue("", ss.str());
}

TEST_F(ConstantFoldingTest, SameStringsSimple) {
  ShouldBeTrue("", "\"foo\" == \"foo\"");
}

TEST_F(ConstantFoldingTest, SameStringsConcat) {
  ShouldBeTrue("", "\"foo\" + \"bar\" == \"fooba\" + \"r\"");
}

TEST_F(ConstantFoldingTest, DiffStrings) {
  ShouldBeFalse("", "\"foo\" + \"bar\" == \"ooba\" + \"r\"");
}

} // namespace types
