#include "joosc.h"

#include <iostream>

#include "ast/ast.h"
#include "ast/print_visitor.h"
#include "base/error.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "types/types.h"
#include "weeder/weeder.h"

using std::cerr;
using std::cout;
using std::endl;
using std::ostream;

using ast::PrintVisitor;
using ast::Program;
using base::ErrorList;
using base::FileSet;
using lexer::LexJoosFiles;
using lexer::StripSkippableTokens;
using lexer::Token;
using parser::Parse;
using types::TypecheckProgram;
using weeder::WeedProgram;

namespace {

bool PrintErrors(const ErrorList& errors, ostream* err, const FileSet* fs) {
  if (errors.Size() > 0) {
    errors.PrintTo(err, base::OutputOptions::kUserOutput, fs);
  }
  return errors.IsFatal();
}

}

sptr<const Program> CompilerFrontend(CompilerStage stage, const FileSet* fs, ErrorList* out) {
  // Lex files.
  vector<vector<Token>> tokens;
  LexJoosFiles(fs, &tokens, out);
  if (out->IsFatal() || stage == CompilerStage::LEX) {
    return nullptr;
  }

  // Strip out comments and whitespace.
  vector<vector<Token>> filtered_tokens;
  StripSkippableTokens(tokens, &filtered_tokens);

  // Look for unsupported tokens.
  FindUnsupportedTokens(tokens, out);
  if (out->IsFatal() || stage == CompilerStage::UNSUPPORTED_TOKS) {
    return nullptr;
  }

  // Parse.
  sptr<const Program> program = Parse(fs, filtered_tokens, out);
  if (out->IsFatal() || stage == CompilerStage::PARSE) {
    return program;
  }

  // Weed.
  program = WeedProgram(fs, program, out);
  if (out->IsFatal() || stage == CompilerStage::WEED) {
    return program;
  }

  // Type-checking.
  program = TypecheckProgram(program, out);
  if (out->IsFatal() || stage == CompilerStage::TYPE_CHECK) {
    return program;
  }

  // Add more implementation here.
  return program;
}

void CompilerBackend(CompilerStage stage, sptr<const ast::Program> prog, ostream* os) {
  ir::Program ir_prog = ir::GenerateIR(prog);
  if (stage == CompilerStage::GEN_IR) {
    return;
  }

  // TODO: Just print the asm.
  ir::Stream stream = ir_prog.units.at(0).streams.at(0);
  for (ir::Op& op : stream.ops) {
    *os << (u32)op.type << " ";
    for (size_t i = op.begin; i < op.end; ++i) {
      *os << stream.args.at(i) << ", ";
    }
    *os << "\n";
  }

  // TODO: asm.
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
      errors.PrintTo(&cerr, base::OutputOptions::kUserOutput, fs);
      return false;
    }
  }
  uptr<FileSet> fs_deleter(fs);
  if (stage == CompilerStage::OPEN_FILES) {
    return true;
  }

  ErrorList errors;
  sptr<const Program> program = CompilerFrontend(stage, fs, &errors);
  if (PrintErrors(errors, err, fs)) {
    return false;
  }
  if (stage == CompilerStage::TYPE_CHECK) {
    return true;
  }

  // TODO.
  CompilerBackend(stage, program, out);

  return true;
}
