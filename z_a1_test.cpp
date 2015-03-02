#include <algorithm>
#include <dirent.h>
#include <iostream>
#include <iterator>

#include "third_party/gtest/gtest.h"

#include "base/error.h"
#include "base/file_walker.h"
#include "joosc.h"

using base::ErrorList;
using base::FileSet;

namespace {

bool IsJavaFile(const string& name) {
  static const string kSuffix = ".java";
  if (name.size() < kSuffix.size()) {
    return false;
  }
  return name.compare(name.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0;
}

bool ListDirRecursive(const string& dir, vector<string>* out) {
  auto cb = [&](const dirent& ent) {
    string basename = string(ent.d_name);
    string fullname = dir + '/' + basename;

    // If its a regular file (i.e. Foo.java) then just append to out.
    if (ent.d_type == DT_REG) {
      out->push_back(fullname);
      return true;
    }

    // If its a not a directory, then we don't care. It's probably a symlink or
    // something weird.
    if (ent.d_type != DT_DIR) {
      return true;
    }

    // If its either of the current directory or the parent directory, then
    // bail to avoid infinite loops.
    if (basename == "." || basename == "..") {
      return true;
    }

    // Otherwise recurse.
    ListDirRecursive(fullname, out);
    return true;
  };

  return base::WalkDir(dir, cb);
}

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

bool CompileInput::GetAllFiles(vector<string>* files) const {
  vector<string> temp;
  bool ok = true;
  if (stdlib_dir != "") {
    ok = ListDirRecursive(stdlib_dir, &temp) && ok;
  }

  if (input_is_dir) {
    ok = ListDirRecursive(input, &temp) && ok;
  } else {
    temp.push_back(input);
  }

  for (const auto& name : temp) {
    if (IsJavaFile(name)) {
      files->push_back(name);
    }
  }

  return ok;
}

// Print a CompileInput as a copy-pastable command-line incantation to run the
// equivalent test.
std::ostream& operator<<(std::ostream& out, const CompileInput& input) {
  out << '"' << "find";
  if (input.stdlib_dir != "") {
    out << ' ' << input.stdlib_dir;
  }

  out << ' ' << input.input << " -type f -name '*.java' | xargs ./joosc" << '"';
  return out;
}

vector<CompileInput> GetInputs(const string& stdlib, const string& dir, CompilerStage stage, std::function<bool(const string&)> pred) {
  vector<CompileInput> inputs;

  auto cb = [&](const dirent& ent) {
    string basename = string(ent.d_name);
    string fullname = dir + '/' + basename;

    // If its a weird file, then bail.
    if (ent.d_type != DT_REG && ent.d_type != DT_DIR) {
      return true;
    }

    // If its either of the current directory or the parent directory, then
    // bail to avoid infinite loops.
    if (ent.d_type == DT_DIR && (basename == "." || basename == "..")) {
      return true;
    }

    // If the client doesn't want it, then bail.
    if (!pred(fullname)) {
      return true;
    }

    // Otherwise, add a new CompileInput.
    inputs.push_back({stdlib, fullname, ent.d_type == DT_DIR, stage});
    return true;
  };

  assert(base::WalkDir(dir, cb));
  return inputs;
}

vector<CompileInput> GetGoodInputs(const string& stdlib, const string& dir, CompilerStage stage) {
  auto pred = [](const string& name) {
    return name.find("Je") == string::npos;
  };
  return GetInputs(stdlib, dir, stage, pred);
}

vector<CompileInput> GetBadInputs(const string& stdlib, const string& dir, CompilerStage stage) {
  auto pred = [](const string& name) {
    return name.find("Je") != string::npos;
  };
  return GetInputs(stdlib, dir, stage, pred);
}

class CompilerSuccessTest2 : public testing::TestWithParam<CompileInput> {};

TEST_P(CompilerSuccessTest2, ShouldCompile) {
  vector<string> files;
  ASSERT_TRUE(GetParam().GetAllFiles(&files));

  stringstream blackhole;
  ASSERT_TRUE(CompilerMain(GetParam().stage, files, &blackhole, &blackhole));
}

class CompilerFailureTest2 : public testing::TestWithParam<CompileInput> {};

TEST_P(CompilerFailureTest2, ShouldNotCompile) {
  vector<string> files;
  ASSERT_TRUE(GetParam().GetAllFiles(&files));

  stringstream blackhole;
  ASSERT_FALSE(CompilerMain(GetParam().stage, files, &blackhole, &blackhole));
}

const static string kStdlib1 = "";
const static string kStdlib2 = "third_party/cs444/stdlib/2.0";
const static string kStdlib3 = "third_party/cs444/stdlib/3.0";
const static string kStdlib4 = "third_party/cs444/stdlib/4.0";
const static string kStdlib5 = "third_party/cs444/stdlib/5.0";

const static string kTest1 = "third_party/cs444/assignment_testcases/a1";
const static string kTest2 = "third_party/cs444/assignment_testcases/a2";
const static string kTest3 = "third_party/cs444/assignment_testcases/a3";
const static string kTest4 = "third_party/cs444/assignment_testcases/a4";
const static string kTest5 = "third_party/cs444/assignment_testcases/a5";

INSTANTIATE_TEST_CASE_P(MarmosetA1, CompilerSuccessTest2,
    testing::ValuesIn(GetGoodInputs(
        kStdlib1, kTest1, CompilerStage::WEED)));
INSTANTIATE_TEST_CASE_P(MarmosetA1, CompilerFailureTest2,
    testing::ValuesIn(GetBadInputs(
        kStdlib1, kTest1, CompilerStage::WEED)));

// TODO: enable.
// INSTANTIATE_TEST_CASE_P(MarmosetA2, CompilerSuccessTest2,
//     testing::ValuesIn(GetGoodInputs(
//         kStdlib2, kTest2, CompilerStage::TYPE_CHECK)));
// INSTANTIATE_TEST_CASE_P(MarmosetA2, CompilerFailureTest2,
//     testing::ValuesIn(GetBadInputs(
//         kStdlib2, kTest2, CompilerStage::TYPE_CHECK)));

}  // namespace
