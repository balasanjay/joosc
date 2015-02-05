#ifndef WEEDER_WEEDER_TEST_H
#define WEEDER_WEEDER_TEST_H

#include "lexer/lexer.h"
#include "parser/parser_internal.h"
#include "third_party/gtest/gtest.h"

namespace weeder {

class WeederTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    parser_.reset();
    tokens.clear();
    fs_.reset();
  }

  virtual void TearDown() {
    parser_.reset();
    tokens.clear();
    fs_.reset();
  }

  void MakeParser(string s) {
    base::ErrorList errors;

    // Create file set.
    base::FileSet* fs;
    ASSERT_TRUE(base::FileSet::Builder().AddStringFile("foo.java", s).Build(
        &fs, &errors));
    fs_.reset(fs);

    // Lex tokens.
    vector<vector<lexer::Token>> alltokens;
    lexer::LexJoosFiles(fs, &alltokens, &errors);

    // Remote comments and whitespace.
    lexer::StripSkippableTokens(alltokens, &tokens);

    // Make sure it worked.
    ASSERT_EQ(1u, tokens.size());
    ASSERT_FALSE(errors.IsFatal());

    parser_.reset(new parser::Parser(fs, fs->Get(0), &tokens[0]));
  }

  unique_ptr<base::FileSet> fs_;
  vector<vector<lexer::Token>> tokens;
  unique_ptr<parser::Parser> parser_;
};

}  // namespace weeder

#endif
