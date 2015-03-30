#include "joosc.h"

#include <fstream>
#include <iostream>

#include "ast/ast.h"
#include "ast/print_visitor.h"
#include "backend/common/offset_table.h"
#include "backend/i386/writer.h"
#include "base/error.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "types/type_info_map.h"
#include "types/types.h"
#include "weeder/weeder.h"

using std::cerr;
using std::cout;
using std::endl;
using std::ofstream;
using std::ostream;

using ast::PrintVisitor;
using ast::Program;
using backend::common::OffsetTable;
using base::ErrorList;
using base::FileSet;
using lexer::LexJoosFiles;
using lexer::StripSkippableTokens;
using lexer::Token;
using parser::Parse;
using types::TypeInfoMap;
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

sptr<const Program> CompilerFrontend(CompilerStage stage, const FileSet* fs, TypeInfoMap* tinfo_out, ErrorList* err_out) {
  // Lex files.
  vector<vector<Token>> tokens;
  LexJoosFiles(fs, &tokens, err_out);
  if (err_out->IsFatal() || stage == CompilerStage::LEX) {
    return nullptr;
  }

  // Strip err_out comments and whitespace.
  vector<vector<Token>> filtered_tokens;
  StripSkippableTokens(tokens, &filtered_tokens);

  // Look for unsupported tokens.
  FindUnsupportedTokens(tokens, err_out);
  if (err_out->IsFatal() || stage == CompilerStage::UNSUPPORTED_TOKS) {
    return nullptr;
  }

  // Parse.
  sptr<const Program> program = Parse(fs, filtered_tokens, err_out);
  if (err_out->IsFatal() || stage == CompilerStage::PARSE) {
    return program;
  }

  // Weed.
  program = WeedProgram(fs, program, err_out);
  if (err_out->IsFatal() || stage == CompilerStage::WEED) {
    return program;
  }

  // Type-checking.
  program = TypecheckProgram(program, tinfo_out, err_out);
  if (err_out->IsFatal() || stage == CompilerStage::TYPE_CHECK) {
    return program;
  }

  // Add more implementation here.
  return program;
}

bool CompilerBackend(CompilerStage stage, sptr<const ast::Program> prog, const string& dir, const TypeInfoMap& tinfo_map, std::ostream* err) {
  ir::Program ir_prog = ir::GenerateIR(prog, tinfo_map);
  if (stage == CompilerStage::GEN_IR) {
    return true;
  }

  // TODO: have a more generic backend mechanism.
  // Generate type sizes, field offsets, and method offsets.
  OffsetTable offset_table = OffsetTable::Build(tinfo_map, 4);

  bool success = true;
  backend::i386::Writer writer(offset_table);
  for (const ir::CompUnit& comp_unit : ir_prog.units) {
    string fname = dir + "/" + comp_unit.filename;

    ofstream out(fname);
    if (!out) {
      // TODO: make error pretty.
      *err << "Could not open output file: " << fname << "\n";
      success = false;
      continue;
    }

    writer.WriteCompUnit(comp_unit, &out);

    out << std::flush;
  }

  do {
    string fname = dir + "/main.s";
    ofstream out(fname);
    if (!out) {
      // TODO: make error pretty.
      *err << "Could not open output file: " << fname << "\n";
      success = false;
      break;
    }

    writer.WriteMain(&out);
    writer.WriteStaticInit(ir_prog, &out);
    out << std::flush;

  } while (false);

  return success;
}

bool CompilerMain(CompilerStage stage, const vector<string>& files, ostream*, ostream* err) {
  // Open files.
  FileSet* fs = nullptr;
  {
    ErrorList errors;
    FileSet::Builder builder;

    // Runtime files.
    builder.AddDiskFile("runtime/__joos_internal__/TypeInfo.java");

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
  TypeInfoMap tinfo_map = TypeInfoMap::Empty();
  sptr<const Program> program = CompilerFrontend(stage, fs, &tinfo_map, &errors);
  if (PrintErrors(errors, err, fs)) {
    return false;
  }
  if (stage == CompilerStage::TYPE_CHECK) {
    return true;
  }

  // TODO.
  return CompilerBackend(stage, program, "output", tinfo_map, err);
}
