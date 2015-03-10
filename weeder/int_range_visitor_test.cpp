#include "weeder/int_range_visitor.h"

#include "weeder/weeder_test.h"

using ast::Expr;
using base::ErrorList;
using parser::internal::Result;

namespace weeder {

class IntRangeVisitorTest : public WeederTest {};

TEST_F(IntRangeVisitorTest, IntTooNegative) {
  MakeParser("-100000000000");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  IntRangeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidIntRangeError(0:1-13)\n", testing::PrintToString(errors));
}

TEST_F(IntRangeVisitorTest, IntExactNegative) {
  MakeParser("-2147483648");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  IntRangeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(IntRangeVisitorTest, IntOneOffNegative) {
  MakeParser("-2147483649");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  IntRangeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidIntRangeError(0:1-11)\n", testing::PrintToString(errors));
}

TEST_F(IntRangeVisitorTest, IntTooPositive) {
  MakeParser("100000000000");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  IntRangeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidIntRangeError(0:0-12)\n", testing::PrintToString(errors));
}

TEST_F(IntRangeVisitorTest, IntExactPositive) {
  MakeParser("2147483647");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  IntRangeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(IntRangeVisitorTest, IntOneOffPositive) {
  MakeParser("2147483648");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  IntRangeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidIntRangeError(0:0-10)\n", testing::PrintToString(errors));
}

TEST_F(IntRangeVisitorTest, Int64Overflow) {
  MakeParser("1000000000000000000000000000000000000000000");
  Result<Expr> expr;
  ASSERT_FALSE(parser_->ParseExpression(&expr).Failed());

  ErrorList errors;
  IntRangeVisitor visitor(&errors);
  auto _ = visitor.Rewrite(expr.Get());

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidIntRangeError(0:0-43)\n", testing::PrintToString(errors));
}

}  // namespace weeder
