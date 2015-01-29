#include "weeder/assignment_visitor.h"
#include "weeder/weeder_test.h"

using base::ErrorList;
using parser::Stmt;
using parser::internal::Result;

namespace weeder {

class AssignmentVisitorTest : public WeederTest {
};

TEST_F(AssignmentVisitorTest, Name) {
  MakeParser("a = 1;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  AssignmentVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(AssignmentVisitorTest, FieldDeref) {
  MakeParser("this.f = 1;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  AssignmentVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(AssignmentVisitorTest, ArrayIndex) {
  MakeParser("a[0] = 1;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  AssignmentVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_FALSE(errors.IsFatal());
}

TEST_F(AssignmentVisitorTest, Fail) {
  MakeParser("a() = 1;");
  Result<Stmt> stmt;
  ASSERT_FALSE(parser_->ParseStmt(&stmt).Failed());

  ErrorList errors;
  AssignmentVisitor visitor(fs_.get(), &errors);
  stmt.Get()->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("InvalidLHSError(0:4)\n", testing::PrintToString(errors));
}

} // namespace weeder
