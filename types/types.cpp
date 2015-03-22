#include "types/types.h"

#include "ast/ast.h"
#include "types/decl_resolver.h"
#include "types/type_info_map.h"
#include "types/typechecker.h"
#include "types/dataflow_visitor.h"
#include "types/typeset.h"

using ast::Program;
using ast::QualifiedName;
using ast::TypeId;
using base::Error;
using base::ErrorList;
using base::MakeError;
using base::OutputOptions;
using base::PosRange;
using lexer::Token;

namespace types {

namespace {

Error* MakeMissingPredefError(const string& msg) {
  return MakeError([=](std::ostream* out, const OutputOptions& opt, const base::FileSet*) {
    if (opt.simple) {
      *out << "MissingPredefError";
      return;
    }

    *out << opt.BoldOn() << opt.Red() << "error: " << opt.ResetColor()
         << "Missing " << msg << "." << opt.BoldOff();
  });
}

bool VerifyTypeSet(const TypeSet& typeset, ErrorList* out) {
  const vector<string> stdlib_types = {
    "java.io.Serializable",
    "java.lang.Boolean",
    "java.lang.Byte",
    "java.lang.Character",
    "java.lang.Cloneable",
    "java.lang.Integer",
    "java.lang.Object",
    "java.lang.Short",
    "java.lang.String",
  };
  bool ok = true;
  for (const string& name : stdlib_types) {
    if (typeset.TryGet(name) == TypeId::kUnassigned) {
      out->Append(MakeMissingPredefError("class " + name));
      ok = false;
    }
  }
  return ok;
}

TypeSet BuildTypeSet(const Program& prog, ErrorList* out) {
  types::TypeSetBuilder builder;
  for (int i = 0; i < prog.CompUnits().Size(); ++i) {
    builder.AddCompUnit(prog.CompUnits().At(i));
  }
  return builder.Build(out);
}

TypeInfoMap BuildTypeInfoMap(const TypeSet& typeset, sptr<const Program> prog,
                             sptr<const Program>* new_prog, ErrorList* error_out) {
  TypeId object_tid = typeset.TryGet("java.lang.Object");
  TypeId serializable_tid = typeset.TryGet("java.io.Serializable");
  TypeId cloneable_tid = typeset.TryGet("java.lang.Cloneable");

  CHECK(object_tid.IsValid());
  CHECK(serializable_tid.IsValid());
  CHECK(cloneable_tid.IsValid());

  TypeInfoMapBuilder builder(object_tid, serializable_tid, cloneable_tid);
  DeclResolver resolver(&builder, typeset, error_out);

  *new_prog = resolver.Rewrite(prog);

  return builder.Build(error_out);
}

}  // namespace

sptr<const Program> TypecheckProgram(sptr<const Program> prog, ErrorList* errors) {
  // Phase 1: Build a typeset.
  TypeSet typeSet = BuildTypeSet(*prog, errors);
  if (!VerifyTypeSet(typeSet, errors)) {
    return prog;
  }

  // Phase 2: Build a type info map.
  TypeInfoMap typeInfo = BuildTypeInfoMap(typeSet, prog, &prog, errors);

  // Phase 3: Typecheck.
  {
    TypeChecker typechecker = TypeChecker(errors)
        .WithTypeSet(typeSet)
        .WithTypeInfoMap(typeInfo);

    prog = typechecker.Rewrite(prog);
  }

  // Don't progress with Dataflow if we have errors so far because pruned
  // return statements will cause false positives.
  if (errors->IsFatal()) {
    return prog;
  }

  // Phase 4: Dataflow Analysis.
  {
    DataflowVisitor dataflow(typeInfo, errors);
    dataflow.Visit(prog);
  }

  return prog;

  // TODO: this function needs to return all the data structures it has built.
}

}  // namespace types
