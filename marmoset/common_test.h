#ifndef MARMOSET_COMMON_H
#define MARMOSET_COMMON_H

#include <algorithm>
#include <dirent.h>
#include <iostream>
#include <iterator>

#include "third_party/gtest/gtest.h"

#include "base/error.h"
#include "base/file_walker.h"
#include "joosc.h"

namespace marmoset {

struct CompileInput {
  bool GetAllFiles(vector<string>* out) const;
  std::ostream& operator<<(std::ostream& out) const;

  // If not "", means a directory to search for .java files.
  string stdlib_dir;

  // The input file(s). Consult input_is_dir to know whether this is a file or
  // directory name.
  string input;
  bool input_is_dir;

  // The compile stage to stop at.
  CompilerStage stage;
};

std::ostream& operator<<(std::ostream& out, const CompileInput& input);

vector<CompileInput> GetGoodInputs(const string& stdlib, const string& dir, CompilerStage stage);

vector<CompileInput> GetBadInputs(const string& stdlib, const string& dir, CompilerStage stage);


class CompilerSuccessTest : public testing::TestWithParam<CompileInput> {};
class CompilerFailureTest : public testing::TestWithParam<CompileInput> {};



}  // namespace marmoset

#endif
