#include "types/typechecker.h"

#include "ast/ast.h"
#include "ast/cloner.h"
#include "types/decl_resolver.h"
#include "types/type_info_map.h"
#include "types/types.h"

using ast::Program;
using base::ErrorList;
using base::FileSet;

namespace types {

namespace {

TypeSet BuildTypeSet(const Program& prog, const FileSet* fs, ErrorList* out) {
  types::TypeSetBuilder typeSetBuilder;
  for (const auto& unit : prog.CompUnits()) {
    vector<string> ns;
    if (unit.Package() != nullptr) {
      ns = unit.Package()->Parts();
    }
    for (const auto& decl : unit.Types()) {
      typeSetBuilder.Put(ns, decl.Name(), decl.NameToken().pos);
    }
  }
  return typeSetBuilder.Build(fs, out);
}

TypeInfoMap BuildTypeInfoMap(const TypeSet& typeset, const Program& prog,
                             const FileSet* fs, uptr<Program>* new_prog,
                             ErrorList* error_out) {
  TypeInfoMapBuilder builder;
  DeclResolver resolver(&builder, typeset, fs, error_out);

  Program* rewritten_prog = prog.AcceptRewriter(&resolver);
  new_prog->reset(rewritten_prog);

  return builder.Build(fs, error_out);
}

}  // namespace

uptr<Program> TypecheckProgram(const Program& prog, const FileSet* fs,
                               ErrorList* out) {
  // Phase 1: build a typeset.
  TypeSet typeset = BuildTypeSet(prog, fs, out);

  // Phase 2: build a type info map.
  uptr<Program> prog2;
  TypeInfoMap typeInfoMap = BuildTypeInfoMap(typeset, prog, fs, &prog2, out);

  ast::Cloner cloner;
  return move(prog2);

  // TODO: this function needs to return all the data structures it has built.
}

}  // namespace types
