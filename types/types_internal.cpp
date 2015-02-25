#include "types/types_internal.h"

#include "ast/ast.h"
#include "base/error.h"
#include "base/macros.h"

using ast::ArrayType;
using ast::PrimitiveType;
using ast::ReferenceType;
using ast::Type;
using ast::TypeId;
using base::Error;
using base::FileSet;
using base::PosRange;

namespace types {

Error* MakeUnknownTypenameError(const FileSet* fs, PosRange pos) {
  return MakeSimplePosRangeError(fs, pos, "UnknownTypenameError",
                                 "Unknown type name.");
}

sptr<const Type> ResolveType(sptr<const Type> type, TypeSet typeset, PosRange* pos_out) {
  const Type* cur = type.get();

  // References.
  if (IS_CONST_PTR(ReferenceType, cur)) {
    const ReferenceType* ref = dynamic_cast<const ReferenceType*>(cur);
    *pos_out = ref->Name().Tokens().back().pos;
    TypeId got = typeset.Get(ref->Name().Parts());
    if (got == cur->GetTypeId()) {
      return type;
    }
    return make_shared<ReferenceType>(ref->Name(), got);
  }

  // Primitives.
  if (IS_CONST_PTR(PrimitiveType, cur)) {
    const PrimitiveType* prim = dynamic_cast<const PrimitiveType*>(cur);
    *pos_out = prim->GetToken().pos;
    TypeId got = typeset.Get({prim->GetToken().TypeInfo().Value()});
    if (got == cur->GetTypeId()) {
      return type;
    }
    return make_shared<PrimitiveType>(prim->GetToken(), got);
  }

  // Arrays.
  assert(IS_CONST_PTR(ArrayType, cur));
  const ArrayType* arr = dynamic_cast<const ArrayType*>(cur);

  sptr<const Type> nested = ResolveType(arr->ElemTypePtr(), typeset, pos_out);
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
