#include "base/error.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "lexer/lexer.h"
#include <iostream>

using base::FileSet;
using lexer::LexJoosFiles;
using lexer::Token;
using std::cerr;
using std::cout;
using std::endl;

class NonAnsiCharError : public base::Error {
  void PrintTo(std::ostream* out, const base::OutputOptions& opt) const override {
    if (opt.simple) {
      *out << "NonAnsiCharError\n";
      return;
    }

    *out << Red(opt) << "error: My user visible message." << ResetFmt(opt) <<
      " Other sub-message.";
  }
};

int main(int argc, char** argv) {
  const int ERROR = 42;

  if (argc != 2) {
    cerr << "usage: joosc <filename>" << endl;
    return ERROR;
  }

  string filename = argv[1];

  base::ErrorList errors;
  FileSet* fs = nullptr;
  if (!FileSet::Builder().AddDiskFile(filename).Build(&fs, &errors)) {
    errors.PrintTo(&cerr, base::OutputOptions::kUserOutput);
    return ERROR;
  }
  unique_ptr<FileSet> fs_deleter(fs);

  vector<vector<Token>> tokens;

  LexJoosFiles(fs, &tokens, &errors);

  if (errors.IsFatal()) {
    errors.PrintTo(&cerr, base::OutputOptions::kUserOutput);
    return ERROR;
  }

  for (auto token : tokens[0]) {
    cout << TokenTypeToString(token.type) << endl;
  }

  return 0;
}
