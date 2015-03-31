#ifndef TYPES_TYPES_TEST
#define TYPES_TYPES_TEST

#include <limits>

#include "ast/ast_fwd.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "third_party/gtest/gtest.h"

#define EXPECT_ERRS(msg) EXPECT_EQ(msg, testing::PrintToString(errors_))
#define EXPECT_NO_ERRS() EXPECT_EQ(0, errors_.Size())
#define PRINT_ERRS() errors_.PrintTo(&std::cerr, base::OutputOptions::kUserOutput, fs_.get())

namespace types {

sptr<const ast::Program> ParseProgramWithStdlib(base::FileSet** fs, const vector<pair<string, string>>& file_contents, base::ErrorList* out);

class TypesTest : public ::testing::Test {
protected:
  sptr<const ast::Program> ParseProgram(const vector<pair<string, string>>& file_contents) {
    base::FileSet* fs;
    sptr<const ast::Program> program = ParseProgramWithStdlib(&fs, file_contents, &errors_);
    fs_.reset(fs);
    return program;
  }

  base::ErrorList errors_;
  uptr<base::FileSet> fs_;
};

}  // namespace types

#endif
