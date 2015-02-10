#include "base/error.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "joosc.h"
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
using std::ostream;
using weeder::WeedProgram;

namespace {

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

bool PrintErrors(const ErrorList& errors, ostream* err) {
  if (errors.Size() > 0) {
    errors.PrintTo(err, base::OutputOptions::kUserOutput);
  }
  return errors.IsFatal();
}

}

bool CompilerMain(CompilerStage stage, const vector<string>& files, ostream* out, ostream* err) {
  // Open files.
  FileSet* fs = nullptr;
  {
    ErrorList errors;
    FileSet::Builder builder;

    for (const auto& file : files) {
      builder.AddDiskFile(file);
    }

    if (!builder.Build(&fs, &errors)) {
      errors.PrintTo(&cerr, base::OutputOptions::kUserOutput);
      return false;
    }
  }
  unique_ptr<FileSet> fs_deleter(fs);
  if (stage == CompilerStage::OPEN_FILES) {
    return true;
  }

  // Lex files.
  vector<vector<Token>> tokens;
  {
    ErrorList errors;
    LexJoosFiles(fs, &tokens, &errors);
    if (PrintErrors(errors, err)) {
      return false;
    }
  }
  if (stage == CompilerStage::LEX) {
    return true;
  }

  // Strip out comments and whitespace.
  vector<vector<Token>> filtered_tokens;
  StripSkippableTokens(tokens, &filtered_tokens);

  // Look for unsupported tokens.
  {
    ErrorList errors;
    FindUnsupportedTokens(fs, tokens, &errors);
    if (PrintErrors(errors, err)) {
      return false;
    }
  }
  if (stage == CompilerStage::UNSUPPORTED_TOKS) {
    return true;
  }

  // Parse.
  unique_ptr<Program> program;
  {
    ErrorList errors;
    unique_ptr<Program> prog = Parse(fs, filtered_tokens, &errors);
    if (PrintErrors(errors, err)) {
      return false;
    }
    program.swap(prog);
  }
  if (stage == CompilerStage::PARSE) {
    return true;
  }

  // Weed.
  {
    ErrorList errors;
    WeedProgram(fs, program.get(), &errors);
    if (PrintErrors(errors, err)) {
      return false;
    }
  }
  if (stage == CompilerStage::WEED) {
    return true;
  }

  // Print out the AST.
  {
    PrintVisitor printer = PrintVisitor::Pretty(out);
    program.get()->Accept(&printer);
  }

  return true;
}
