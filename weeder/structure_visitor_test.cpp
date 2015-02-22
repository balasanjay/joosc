#include "weeder/structure_visitor.h"

#include "weeder/weeder_test.h"

using std::move;

using ast::CompUnit;
using ast::Program;
using ast::Stmt;
using base::ErrorList;
using base::SharedPtrVector;
using base::UniquePtrVector;
using parser::internal::Result;

namespace weeder {

class StructureVisitorTest : public WeederTest {
 protected:
  sptr<Program> ParseProgram(const string& program) {
    MakeParser(program);

    Result<CompUnit> unit;
    if (!parser_->ParseCompUnit(&unit)) {
      return nullptr;
    }

    SharedPtrVector<const CompUnit> units;
    units.Append(unit.Get());

    return make_shared<Program>(units);
  }
};

TEST_F(StructureVisitorTest, MultipleTypesInFile) {
  sptr<Program> prog = ParseProgram("class foo{}; interface bar{}");
  ASSERT_TRUE(prog != nullptr);

  ErrorList errors;
  StructureVisitor visitor(fs_.get(), &errors);
  visitor.Visit(prog);

  string expected =
      "MultipleTypesPerCompUnitError(0:6-9)\n"
      "MultipleTypesPerCompUnitError(0:23-26)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(StructureVisitorTest, DifferentFileName) {
  sptr<Program> prog = ParseProgram("class bar{}");
  ASSERT_TRUE(prog != nullptr);

  ErrorList errors;
  StructureVisitor visitor(fs_.get(), &errors);
  visitor.Visit(prog);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("IncorrectFileNameError(0:6-9)\n", testing::PrintToString(errors));
}

TEST_F(StructureVisitorTest, StructureOk) {
  sptr<Program> prog = ParseProgram("class foo{}");
  ASSERT_TRUE(prog != nullptr);

  ErrorList errors;
  StructureVisitor visitor(fs_.get(), &errors);
  visitor.Visit(prog);

  EXPECT_EQ(errors.Size(), 0);
}

}  // namespace weeder
