#include <algorithm>
#include <dirent.h>
#include <iostream>
#include <iterator>

#include "ast/ast.h"
#include "base/error.h"
#include "joosc.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "third_party/gtest/gtest.h"
#include "weeder/weeder.h"

using std::back_inserter;
using std::cerr;
using std::copy_if;
using std::cout;

using ast::Program;
using base::ErrorList;
using base::FileSet;
using base::PosRange;
using lexer::FindUnsupportedTokens;
using lexer::StripSkippableTokens;
using lexer::Token;
using parser::Parse;

namespace {

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
  ASSERT_TRUE(CompilerMain(CompilerStage::WEED, {GetParam()}, &cout, &cerr));
}

class CompilerFailureTest : public testing::TestWithParam<string> {};

TEST_P(CompilerFailureTest, ShouldNotCompile) {
  stringstream blackhole;
  ASSERT_FALSE(CompilerMain(CompilerStage::WEED, {GetParam()}, &blackhole, &blackhole));
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

}  // namespace
