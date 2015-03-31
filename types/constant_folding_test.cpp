#include "types/types_test.h"

namespace types {

class ConstantFoldingTest: public TypesTest {
  enum class FoldResult {
    False = 0,
    True = 1,
    Unknown = -1
  };

  // Relies on reachability to check whether the constant
  // was folded at compile-time.
  FoldResult ReachabilityTest(string init, string expr) {
    ParseProgram({
      {"A.java", "public class A { public int a() { " + init + "while(" + expr + ") return 1; } }"}
    });
    switch (errors_.Size()) {
      case 0:
        return FoldResult::True;
      case 1:
        // Expecting method needs a return.
        return FoldResult::Unknown;
      case 2:
        // Expecting unreachable while body and method needs a return.
        return FoldResult::False;
      default:
        CHECK(false);
    }
  }

public:
  void ShouldBeTrue(string init, string expr) {
    EXPECT_EQ(
      FoldResult::True,
      ReachabilityTest(init, expr));
  }

  void ShouldBeFalse(string init, string expr) {
    EXPECT_EQ(
      FoldResult::False,
      ReachabilityTest(init, expr));
  }

  void ShouldBeUnknown(string init, string expr) {
    EXPECT_EQ(
      FoldResult::Unknown,
      ReachabilityTest(init, expr));
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
  ss << std::numeric_limits<i32>::max()
     << " + 1 == "
     << std::numeric_limits<i32>::min();
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

TEST_F(ConstantFoldingTest, StringifyInt) {
  ShouldBeTrue("", "\"foo\" + -12 == \"foo-12\"");
}

TEST_F(ConstantFoldingTest, StringifyBools) {
  ShouldBeTrue("", "true + \"foo\" + false == \"truefoofalse\"");
}

TEST_F(ConstantFoldingTest, StringifyChars) {
  ShouldBeTrue("", "'a' + \"foo\" + 'b' == \"afoob\"");
}

TEST_F(ConstantFoldingTest, NoStringifyNull) {
  ShouldBeUnknown("", "null + \"y\" == \"nully\"");
}

} // namespace types
