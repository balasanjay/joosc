#include "types/types.h"

#include "ast/ast.h"
#include "types/decl_resolver.h"
#include "types/type_info_map.h"
#include "types/typechecker.h"
#include "types/typeset.h"

using ast::Program;
using ast::QualifiedName;
using ast::TypeId;
using base::Error;
using base::ErrorList;
using base::FileSet;
using base::MakeError;
using base::OutputOptions;
using base::PosRange;
using lexer::Token;

namespace types {

namespace {

Error* MakeMissingPredefError(const string& msg) {
  return MakeError([=](std::ostream* out, const OutputOptions& opt) {
    if (opt.simple) {
      *out << "MissingPredefError";
      return;
    }

    *out << opt.BoldOn() << opt.Red() << "error: " << opt.ResetColor()
         << "Missing " << msg << "." << opt.BoldOff();
  });
}

bool VerifyTypeSet(const TypeSet& typeset, ErrorList* out) {
  const vector<string> lang_classes =
    {"Boolean", "Byte", "Character", "Integer", "Object", "Short", "String"};
  bool ok = true;
  for (const string& name : lang_classes) {
    const string& full_name = "java.lang." + name;
    if (typeset.TryGet(full_name) == TypeId::kUnassigned) {
      out->Append(MakeMissingPredefError("class " + full_name));
      ok = false;
    }
  }
  return ok;
}

vector<TypeSetBuilder::Elem> ExtractElems(const QualifiedName& name) {
  vector<TypeSetBuilder::Elem> v;
  v.reserve(name.Parts().size());

  for (size_t i = 0; i < name.Parts().size(); ++i) {
    v.push_back({name.Parts().at(i), name.Tokens().at(i).pos});
  }

  return v;
}

TypeSet BuildTypeSet(const Program& prog, const FileSet* fs, ErrorList* out) {
  types::TypeSetBuilder typeSetBuilder;
  for (const auto& unit : prog.CompUnits()) {
    vector<TypeSetBuilder::Elem> pkg;
    if (unit.PackagePtr() != nullptr) {
      pkg = ExtractElems(*unit.PackagePtr());
      typeSetBuilder.AddPackage(pkg);
    }
    for (const auto& decl : unit.Types()) {
      typeSetBuilder.AddType(pkg, {decl.Name(), decl.NameToken().pos});
    }
  }
  return typeSetBuilder.Build(fs, out);
}

TypeInfoMap BuildTypeInfoMap(const TypeSet& typeset, sptr<const Program> prog,
                             const FileSet* fs, sptr<const Program>* new_prog,
                             ErrorList* error_out) {
  TypeId object_tid = typeset.TryGet("java.lang.Object");
  CHECK(object_tid.IsValid());
  TypeInfoMapBuilder builder(fs, object_tid);
  DeclResolver resolver(&builder, typeset, fs, error_out);

  *new_prog = resolver.Rewrite(prog);

  return builder.Build(typeset, error_out);
}

}  // namespace

sptr<const Program> TypecheckProgram(sptr<const Program> prog, const FileSet* fs,
                               ErrorList* out) {
  // Phase 1: build a typeset.
  TypeSet typeSet = BuildTypeSet(*prog, fs, out);
  if (!VerifyTypeSet(typeSet, out)) {
    return prog;
  }

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
