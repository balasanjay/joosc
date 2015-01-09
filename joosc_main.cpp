#include "base/fileset.h"
#include "lexer/lexer.h"
#include <iostream>

using base::FileSet;
using lexer::Error;
using lexer::LexJoosFiles;
using lexer::Token;
using std::cerr;
using std::cout;
using std::endl;

int main(int argc, char** argv) {
  const int ERROR = 42;

  if (argc != 2) {
    cerr << "usage: joosc <filename>" << endl;
    return ERROR;
  }

  string filename = argv[1];

  FileSet* fs = nullptr;
  if (!FileSet::Builder().AddDiskFile(filename).Build(&fs)) {
    cerr << "Couldn't read file" << endl;
    return ERROR;
  }

  vector<vector<Token>> tokens;
  vector<Error> errors;

  LexJoosFiles(fs, &tokens, &errors);

  if (errors.size() > 0) {
    cerr << "error" << endl;
    return ERROR;
  }

  for (auto token : tokens[0]) {
    cout << TokenTypeToString(token.type) << endl;
  }

  return 0;
}
