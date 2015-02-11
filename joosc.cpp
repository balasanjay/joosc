#include "base/error.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "joosc.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "parser/print_visitor.h"
#include "weeder/weeder.h"
#include "typing/rewriter.h"
#include "typing/fun_rewriter.h"
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
  uptr<FileSet> fs_deleter(fs);
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
  uptr<Program> program;
  {
    ErrorList errors;
    uptr<Program> prog = Parse(fs, filtered_tokens, &errors);
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

  // TODO.
  //parser::FunRewriter rewriter;
  //program.reset(program.get()->Rewrite(&rewriter));

  // Print out the AST.
  {
    PrintVisitor printer = PrintVisitor::Josh(out);
    program.get()->AcceptVisitor(&printer);
  }

  return true;
}
