#include "marmoset/common_test.h"

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

namespace marmoset {

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
    // Also accept symlinked files (for use with Blaze).
    if (ent.d_type == DT_REG || ent.d_type == DT_LNK) {
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

  CHECK(base::WalkDir(dir, cb));
  return inputs;
}

} // namespace

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

TEST_P(CompilerSuccessTest, ShouldCompile) {
  vector<string> files;
  ASSERT_TRUE(GetParam().GetAllFiles(&files));

  stringstream blackhole;
  ASSERT_TRUE(CompilerMain(GetParam().stage, files, &std::cerr, &std::cerr));
}

TEST_P(CompilerFailureTest, ShouldNotCompile) {
  vector<string> files;
  ASSERT_TRUE(GetParam().GetAllFiles(&files));

  stringstream blackhole;
  ASSERT_FALSE(CompilerMain(GetParam().stage, files, &blackhole, &blackhole));
}

}  // namespace marmoset
