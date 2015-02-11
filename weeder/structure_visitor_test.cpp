#include "weeder/structure_visitor.h"
#include "weeder/weeder_test.h"

using std::move;
using parser::Program;
using parser::CompUnit;
using base::ErrorList;
using parser::Stmt;
using parser::internal::Result;
using base::UniquePtrVector;

namespace weeder {

class StructureVisitorTest : public WeederTest {
 protected:
  uptr<Program> ParseProgram(const string& program) {
    MakeParser(program);

    Result<CompUnit> unit;
    if (!parser_->ParseCompUnit(&unit)) {
      return nullptr;
    }

    UniquePtrVector<CompUnit> units;
    units.Append(unit.Release());

    return uptr<Program>(new Program(move(units)));
  }
};

TEST_F(StructureVisitorTest, MultipleTypesInFile) {
  uptr<Program> prog = ParseProgram("class foo{}; interface bar{}");
  ASSERT_TRUE(prog != nullptr);

  ErrorList errors;
  StructureVisitor visitor(fs_.get(), &errors);
  prog->Accept(&visitor);

  string expected =
      "MultipleTypesPerCompUnitError(0:6-9)\n"
      "MultipleTypesPerCompUnitError(0:23-26)\n";

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ(expected, testing::PrintToString(errors));
}

TEST_F(StructureVisitorTest, DifferentFileName) {
  uptr<Program> prog = ParseProgram("class bar{}");
  ASSERT_TRUE(prog != nullptr);

  ErrorList errors;
  StructureVisitor visitor(fs_.get(), &errors);
  prog->Accept(&visitor);

  EXPECT_TRUE(errors.IsFatal());
  EXPECT_EQ("IncorrectFileNameError(0:6-9)\n", testing::PrintToString(errors));
}

TEST_F(StructureVisitorTest, StructureOk) {
  uptr<Program> prog = ParseProgram("class foo{}");
  ASSERT_TRUE(prog != nullptr);

  ErrorList errors;
  StructureVisitor visitor(fs_.get(), &errors);
  prog->Accept(&visitor);

  EXPECT_EQ(errors.Size(), 0);
}

}  // namespace weeder
