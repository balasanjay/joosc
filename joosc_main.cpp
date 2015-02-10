<<<<<<< HEAD
#include "base/error.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "parser/print_visitor.h"
#include "types/types.h"
#include "weeder/weeder.h"
#include <iostream>

using base::ErrorList;
using types::TypeSet;
using base::FileSet;
using lexer::LexJoosFiles;
using lexer::StripSkippableTokens;
using lexer::Token;
using parser::Parse;
using parser::PrintVisitor;
using parser::Program;
using std::cerr;
=======
#include "joosc.h"
#include <iostream>

>>>>>>> refactor_main
using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char** argv) {
  const int ERROR = 42;

  if (argc < 2) {
    cerr << "usage: joosc <filename>..." << endl;
    return ERROR;
  }

  vector<string> files;
  for (int i = 1; i < argc; ++i) {
    files.emplace_back(argv[i]);
  }

  bool success = CompilerMain(CompilerStage::ALL, files, &cout, &cerr);
  int retcode = success ? 0 : ERROR;
  return retcode;
}
