#include "weeder/call_visitor.h"
#include "weeder/weeder_test.h"

using base::ErrorList;
using parser::Stmt;
using parser::internal::Result;

namespace weeder {

class CallVisitorTest : public WeederTest {
};

TEST_F(CallVisitorTest, Name) {
  MakeParser("a(1);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  CallVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(CallVisitorTest, FieldDeref) {
  MakeParser("this.a(1);");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  CallVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(CallVisitorTest, Fail) {
  MakeParser("a()();");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  CallVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidCallError(0:3)\n", testing::PrintToString(errors));
}

} // namespace weeder