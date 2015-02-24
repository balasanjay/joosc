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

TypeId ResolveType(const Type& type, TypeSet typeset, PosRange* pos_out) {
  const Type* cur = &type;
  if (IS_CONST_PTR(ReferenceType, cur)) {
    const ReferenceType* ref = dynamic_cast<const ReferenceType*>(cur);
    *pos_out = ref->Name().Tokens().back().pos;
    return typeset.Get(ref->Name().Parts());
  } else if (IS_CONST_PTR(PrimitiveType, cur)) {
    const PrimitiveType* prim = dynamic_cast<const PrimitiveType*>(cur);
    *pos_out = prim->GetToken().pos;
    return typeset.Get({prim->GetToken().TypeInfo().Value()});
  }

  assert(IS_CONST_PTR(ArrayType, cur));
  const ArrayType* arr = dynamic_cast<const ArrayType*>(cur);
  TypeId nested = ResolveType(arr->ElemType(), typeset, pos_out);
  return TypeId{nested.base, nested.ndims + 1};
}


} // namespace types
