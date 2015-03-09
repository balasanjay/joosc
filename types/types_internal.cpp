#include "types/types_internal.h"

#include "ast/ast.h"
#include "base/error.h"
#include "base/macros.h"

using std::ostream;

using ast::ArrayType;
using ast::PrimitiveType;
using ast::ReferenceType;
using ast::Type;
using ast::TypeId;
using base::DiagnosticClass;
using base::Error;
using base::ErrorList;
using base::FileSet;
using base::MakeError;
using base::OutputOptions;
using base::PosRange;

namespace types {

Error* MakeUnknownTypenameError(const FileSet* fs, PosRange pos) {
  return MakeSimplePosRangeError(fs, pos, "UnknownTypenameError",
                                 "Unknown type name.");
}

Error* MakeDuplicateDefinitionError(const FileSet* fs, const vector<PosRange> dupes, const string& main_message, const string& name) {
  return MakeError([=](ostream* out, const OutputOptions& opt) {
    if (opt.simple) {
      *out << name << ": [";
      for (const auto& dupe : dupes) {
        *out << dupe << ",";
      }
      *out << ']';
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, dupes.at(0), DiagnosticClass::ERROR, main_message);
    PrintRangePtr(out, opt, fs, dupes.at(0));
    for (uint i = 1; i < dupes.size(); ++i) {
      *out << '\n';
      PosRange pos = dupes.at(i);
      PrintDiagnosticHeader(out, opt, fs, pos, DiagnosticClass::INFO, "Also declared here.");
      PrintRangePtr(out, opt, fs, pos);
    }
  });
}

Error* MakeDuplicateInheritanceError(const FileSet* fs, bool is_extends, PosRange pos, TypeId base_tid, TypeId inheriting_tid) {
  stringstream ss;
  ss << "Type " << base_tid.base << " ";
  if (is_extends) {
    ss << "extends";
  } else {
    ss << "implements";
  }
  ss << " " << inheriting_tid.base << " twice.";
  return MakeSimplePosRangeError(fs, pos, "DuplicateInheritanceError", ss.str());
}

sptr<const Type> ResolveType(sptr<const Type> type, const TypeSet& typeset, ErrorList* errors) {
  const Type* cur = type.get();

  // References.
  if (IS_CONST_PTR(ReferenceType, cur)) {
    const ReferenceType* ref = dynamic_cast<const ReferenceType*>(cur);
    PosRange pos = ref->Name().Tokens().front().pos;
    pos.end = ref->Name().Tokens().back().pos.end;
    TypeId got = typeset.Get(ref->Name().Name(), pos, errors);
    if (got == cur->GetTypeId()) {
      return type;
    }
    return make_shared<ReferenceType>(ref->Name(), got);
  }

  // Primitives.
  if (IS_CONST_PTR(PrimitiveType, cur)) {
    const PrimitiveType* prim = dynamic_cast<const PrimitiveType*>(cur);
    TypeId got = typeset.Get(prim->GetToken().TypeInfo().Value(), prim->GetToken().pos, errors);
    if (got == cur->GetTypeId()) {
      return type;
    }
    return make_shared<PrimitiveType>(prim->GetToken(), got);
  }

  // Arrays.
  CHECK(IS_CONST_PTR(ArrayType, cur));
  const ArrayType* arr = dynamic_cast<const ArrayType*>(cur);

  sptr<const Type> nested = ResolveType(arr->ElemTypePtr(), typeset, errors);
  TypeId tid = nested->GetTypeId();
  if (tid.IsValid()) {
    tid = TypeId{tid.base, tid.ndims + 1};
  }

  if (nested == arr->ElemTypePtr() && tid == arr->GetTypeId()) {
    return type;
  }
  return make_shared<ArrayType>(nested, arr->Lbrack(), arr->Rbrack(), tid);
}


} // namespace types
