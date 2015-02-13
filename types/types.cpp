#include "types/types.h"

#include "ast/ast.h"
#include "types/decl_resolver.h"
#include "types/type_info_map.h"
#include "types/typeset.h"

using ast::Program;
using base::ErrorList;
using base::FileSet;

namespace types {

namespace {

TypeSet BuildTypeSet(const Program& prog, const FileSet* fs, ErrorList* out) {
  types::TypeSetBuilder typeSetBuilder;
  for (const auto& unit : prog.CompUnits()) {
    vector<string> ns;
    if (unit.PackagePtr() != nullptr) {
      ns = unit.PackagePtr()->Parts();
    }
    for (const auto& decl : unit.Types()) {
      typeSetBuilder.Put(ns, decl.Name(), decl.NameToken().pos);
    }
  }
  return typeSetBuilder.Build(fs, out);
}

TypeInfoMap BuildTypeInfoMap(const TypeSet& typeset, sptr<const Program> prog,
                             const FileSet* fs, sptr<const Program>* new_prog,
                             ErrorList* error_out) {
  TypeInfoMapBuilder builder;
  DeclResolver resolver(&builder, typeset, fs, error_out);

  *new_prog = Visit(&resolver, prog);

  return builder.Build(fs, error_out);
}

}  // namespace

sptr<const Program> TypecheckProgram(sptr<const Program> prog, const FileSet* fs,
                               ErrorList* out) {
  // Phase 1: build a typeset.
  TypeSet typeset = BuildTypeSet(*prog, fs, out);

  // Phase 2: build a type info map.
  TypeInfoMap typeInfoMap = BuildTypeInfoMap(typeset, prog, fs, &prog, out);

  return prog;

  // TODO: this function needs to return all the data structures it has built.
}

}  // namespace types
