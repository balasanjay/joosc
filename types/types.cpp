#include "types/types.h"

#include "ast/ast.h"
#include "types/decl_resolver.h"
#include "types/type_info_map.h"
#include "types/typechecker.h"
#include "types/typeset.h"

using ast::Program;
using base::ErrorList;
using base::FileSet;
using base::PosRange;
using lexer::Token;

namespace types {

namespace {

vector<PosRange> ExtractPosRanges(const vector<Token> tokens) {
  vector<PosRange> v;
  v.reserve(tokens.size());
  for (Token token : tokens) {
    v.push_back(token.pos);
  }
  return v;
}

TypeSet BuildTypeSet(const Program& prog, const FileSet* fs, ErrorList* out) {
  types::TypeSetBuilder typeSetBuilder;
  for (const auto& unit : prog.CompUnits()) {
    vector<string> pkg;
    vector<PosRange> pkgpos;
    if (unit.PackagePtr() != nullptr) {
      pkg = unit.PackagePtr()->Parts();
      pkgpos = ExtractPosRanges(unit.PackagePtr()->Tokens());
      typeSetBuilder.AddPackage(pkg, pkgpos);
    }
    for (const auto& decl : unit.Types()) {
      typeSetBuilder.AddType(pkg, pkgpos, decl.Name(), decl.NameToken().pos);
    }
  }
  return typeSetBuilder.Build(fs, out);
}

TypeInfoMap BuildTypeInfoMap(const TypeSet& typeset, sptr<const Program> prog,
                             const FileSet* fs, sptr<const Program>* new_prog,
                             ErrorList* error_out) {
  TypeInfoMapBuilder builder;
  DeclResolver resolver(&builder, typeset, fs, error_out);

  *new_prog = resolver.Rewrite(prog);

  return builder.Build(fs, error_out);
}

}  // namespace

sptr<const Program> TypecheckProgram(sptr<const Program> prog, const FileSet* fs,
                               ErrorList* out) {
  // Phase 1: build a typeset.
  TypeSet typeSet = BuildTypeSet(*prog, fs, out);

  // Phase 2: build a type info map.
  TypeInfoMap typeInfo = BuildTypeInfoMap(typeSet, prog, fs, &prog, out);

  // Phase 3: typecheck.
  {
    TypeChecker typechecker = TypeChecker(fs, out)
        .WithTypeSet(typeSet)
        .WithTypeInfoMap(typeInfo);

    prog = typechecker.Rewrite(prog);
  }

  return prog;

  // TODO: this function needs to return all the data structures it has built.
}

}  // namespace types
