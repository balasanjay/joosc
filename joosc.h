#ifndef JOOSC_H
#define JOOSC_H

#include "ast/ast_fwd.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "ir/ir_generator.h"

// A stage of the compiler. Note that each constant implicitly includes all
// prior constants. That is to say, LEX implicitly means to OPEN_FILES then
// LEX.
enum class CompilerStage {
  OPEN_FILES,
  LEX,
  UNSUPPORTED_TOKS,
  PARSE,
  WEED,
  TYPE_CHECK,
  GEN_IR,
  GEN_ASM,

  ALL,
};

// Run the compiler up to and including the indicated stage. The second
// argument is a list of files to compile.
bool CompilerMain(CompilerStage stage, const vector<string>& files,
    std::ostream* out, std::ostream* err);

sptr<const ast::Program> CompilerFrontend(CompilerStage stage, const base::FileSet* fs, base::ErrorList* out);

bool CompilerBackend(CompilerStage stage, sptr<const ast::Program> prog, const string& dir, std::ostream* err);

#endif
