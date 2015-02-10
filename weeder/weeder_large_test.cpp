#include "base/error.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "third_party/gtest/gtest.h"
#include "weeder/weeder.h"

#include <algorithm>
#include <iterator>
#include <dirent.h>

using base::ErrorList;
using base::FileSet;
using base::PosRange;
using lexer::StripSkippableTokens;
using lexer::FindUnsupportedTokens;
using lexer::Token;
using parser::Parse;
using parser::Program;
using std::back_inserter;
using std::copy_if;

namespace weeder {

bool ListDir(const string& dirName, vector<string>* files) {
  DIR* dir = opendir(dirName.c_str());
  if (dir == nullptr) {
    return false;
  }

  while (true) {
    struct dirent* ent = readdir(dir);
    if (ent == nullptr) {
      break;
    }
    if (ent->d_type != DT_REG) {
      continue;
    }
    files->push_back(dirName + '/' + string(ent->d_name));
  }
  closedir(dir);
  return true;
}

class CompilerSuccessTest : public testing::TestWithParam<string> {};

TEST_P(CompilerSuccessTest, ShouldCompile) {
  FileSet::Builder builder = FileSet::Builder().AddDiskFile(GetParam());

  FileSet* fs;
  ErrorList errors;
  vector<vector<Token>> tokens;

  ASSERT_TRUE(builder.Build(&fs, &errors));
  unique_ptr<FileSet> fs_deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.Size() == 0);

  vector<vector<Token>> filtered_tokens;
  StripSkippableTokens(tokens, &filtered_tokens);

  FindUnsupportedTokens(fs, filtered_tokens, &errors);
  ASSERT_TRUE(errors.Size() == 0);

  unique_ptr<Program> prog = Parse(fs, filtered_tokens, &errors);
  if (errors.Size() != 0) {
    errors.PrintTo(&std::cerr, base::OutputOptions::kUserOutput);
    ASSERT_TRUE(false);
  }
  ASSERT_TRUE(prog != nullptr);

  WeedProgram(fs, prog.get(), &errors);
  if (errors.Size() != 0) {
    errors.PrintTo(&std::cerr, base::OutputOptions::kUserOutput);
    ASSERT_TRUE(false);
  }
  ASSERT_TRUE(prog != nullptr);
}

class CompilerFailureTest : public testing::TestWithParam<string> {};

TEST_P(CompilerFailureTest, ShouldNotCompile) {
  FileSet::Builder builder = FileSet::Builder().AddDiskFile(GetParam());

  FileSet* fs;
  ErrorList errors;
  vector<vector<Token>> tokens;

  ASSERT_TRUE(builder.Build(&fs, &errors));
  unique_ptr<FileSet> fs_deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);
  if (errors.IsFatal()) {
    return;
  }

  vector<vector<Token>> filtered_tokens;
  StripSkippableTokens(tokens, &filtered_tokens);

  FindUnsupportedTokens(fs, filtered_tokens, &errors);
  if (errors.IsFatal()) {
    return;
  }

  unique_ptr<Program> prog = Parse(fs, filtered_tokens, &errors);
  if (errors.IsFatal()) {
    return;
  }

  WeedProgram(fs, prog.get(), &errors);
  if (errors.IsFatal()) {
    return;
  }

  // Shouldn't have made it here.
  ASSERT_TRUE(false);
}

vector<string> SuccessFiles() {
  vector<string> files;
  assert(ListDir("third_party/cs444/assignment_testcases/a1", &files));

  vector<string> filtered;
  copy_if(files.begin(), files.end(), back_inserter(filtered),
          [](const string& file) { return file.find("Je") == string::npos; });

  return filtered;
}

vector<string> FailureFiles() {
  vector<string> files;
  assert(ListDir("third_party/cs444/assignment_testcases/a1", &files));

  vector<string> filtered;
  copy_if(files.begin(), files.end(), back_inserter(filtered),
          [](const string& file) { return file.find("Je") != string::npos; });

  return filtered;
}

INSTANTIATE_TEST_CASE_P(MarmosetTests, CompilerSuccessTest,
                        testing::ValuesIn(SuccessFiles()));
INSTANTIATE_TEST_CASE_P(MarmosetTests, CompilerFailureTest,
                        testing::ValuesIn(FailureFiles()));

}  // namespace weeder
