#include "base/error.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "parser/print_visitor.h"
#include "weeder/weeder.h"
#include <iostream>

using base::ErrorList;
using base::FileSet;
using lexer::LexJoosFiles;
using lexer::StripSkippableTokens;
using lexer::Token;
using parser::Parse;
using parser::PrintVisitor;
using parser::Program;
using std::cerr;
using std::cout;
using std::endl;
using weeder::WeedProgram;

class NonAnsiCharError : public base::Error {
  void PrintTo(std::ostream* out,
               const base::OutputOptions& opt) const override {
    if (opt.simple) {
      *out << "NonAnsiCharError\n";
      return;
    }

    *out << Red(opt) << "error: My user visible message." << ResetFmt(opt)
         << " Other sub-message.";
  }
};

bool PrintErrors(const ErrorList& errors) {
  if (errors.Size() > 0) {
    errors.PrintTo(&cerr, base::OutputOptions::kUserOutput);
  }
  return errors.IsFatal();
}

int main(int argc, char** argv) {
  srand(time(0));
  const int ERROR = 42;

  if (argc < 2) {
    cerr << "usage: joosc <filename>..." << endl;
    return ERROR;
  }

  string filename = argv[1];

  // Open files.
  FileSet* fs = nullptr;
  {
    ErrorList errors;
    FileSet::Builder builder;

    for (int i = 1; i < argc; i++) {
      string filename = argv[i];
      builder.AddDiskFile(filename);
    }

    if (!builder.Build(&fs, &errors)) {
      errors.PrintTo(&cerr, base::OutputOptions::kUserOutput);
      return ERROR;
    }
  }
  unique_ptr<FileSet> fs_deleter(fs);

  // Lex files.
  vector<vector<Token>> tokens;
  {
    ErrorList errors;
    LexJoosFiles(fs, &tokens, &errors);
    if (PrintErrors(errors)) {
      return ERROR;
    }
  }

  // Strip out comments and whitespace.
  vector<vector<Token>> filtered_tokens;
  StripSkippableTokens(tokens, &filtered_tokens);

  // Look for unsupported tokens.
  {
    ErrorList errors;
    FindUnsupportedTokens(fs, tokens, &errors);
    if (PrintErrors(errors)) {
      return ERROR;
    }
  }

  // Parse.
  unique_ptr<Program> program;
  {
    ErrorList errors;
    unique_ptr<Program> prog = Parse(fs, filtered_tokens, &errors);
    if (PrintErrors(errors)) {
      return ERROR;
    }
    program.swap(prog);
  }

  // Weed.
  {
    ErrorList errors;
    WeedProgram(fs, program.get(), &errors);
    if (PrintErrors(errors)) {
      return ERROR;
    }
  }

  // Print out the AST.
  {
    PrintVisitor printer = PrintVisitor::Josh(&cout);
    program.get()->Accept(&printer);
  }

  return 0;
}
