#include "types/typechecker.h"

#include "ast/ast.h"
#include "base/error.h"
#include "types/types_internal.h"

using std::initializer_list;

using ast::Type;
using ast::TypeId;
using base::PosRange;

namespace types {

sptr<const Type> TypeChecker::MustResolveType(sptr<const Type> type) {
  sptr<const Type> ret = ResolveType(type, typeset_, errors_);
  if (ret->GetTypeId().IsValid()) {
    return ret;
  }
  return nullptr;
}

TypeId TypeChecker::JavaLangType(const string& name) const {
  return typeset_.TryGet("java.lang." + name);
}

bool TypeChecker::IsNumeric(TypeId tid) const {
  // Arrays are not numeric.
  if (tid.ndims != 0) {
    return false;
  }
  switch (tid.base) {
    case TypeId::kByteBase:
    case TypeId::kCharBase:
    case TypeId::kShortBase:
    case TypeId::kIntBase:
      return true;
    default:
      return false;
  }
}

bool TypeChecker::IsPrimitive(TypeId tid) const {
  // All arrays are reference types.
  if (tid.ndims != 0) {
    return false;
  }
  switch (tid.base) {
    case TypeId::kBoolBase:
    case TypeId::kByteBase:
    case TypeId::kCharBase:
    case TypeId::kShortBase:
    case TypeId::kIntBase:
      return true;
    default:
      return false;
  }
}

bool TypeChecker::IsReference(TypeId tid) const {
  // All arrays are reference types.
  if (tid.ndims > 0) {
    return true;
  }

  // All primitive types are not reference types.
  if (IsPrimitive(tid)) {
    return false;
  }

  // Null is a reference type.
  if (tid.base == TypeId::kNullBase) {
    return true;
  }

  return tid.base >= TypeId::kFirstRefTypeBase;
}

bool IsOneOf(TypeId::Base base, initializer_list<TypeId::Base> allowed) {
  for (auto b : allowed) {
    if (b == base) {
      return true;
    }
  }
  return false;
}

// Returns true iff an assignment `lhs x = (rhs)y' is a primitive widening
// conversion.
bool TypeChecker::IsPrimitiveWidening(TypeId lhs, TypeId rhs) const {
  if (!IsNumeric(lhs) || !IsNumeric(rhs)) {
    return false;
  }

  switch (rhs.base) {
    case TypeId::kByteBase:
      return IsOneOf(lhs.base, {TypeId::kShortBase, TypeId::kIntBase});
    case TypeId::kShortBase:
    case TypeId::kCharBase:
      return lhs.base == TypeId::kIntBase;
    case TypeId::kIntBase: return false;
    default: throw; // Should be unreachable.
  }
}

// Returns true iff an assignment `lhs x = (rhs)y' is a primitive narrowing
// conversion.
bool TypeChecker::IsPrimitiveNarrowing(TypeId lhs, TypeId rhs) const {
  if (!IsNumeric(lhs) || !IsNumeric(rhs)) {
    return false;
  }

  switch (rhs.base) {
    case TypeId::kByteBase:
      return lhs.base == TypeId::kCharBase;
    case TypeId::kShortBase:
      return IsOneOf(lhs.base, {TypeId::kByteBase, TypeId::kCharBase});
    case TypeId::kCharBase:
      return IsOneOf(lhs.base, {TypeId::kByteBase, TypeId::kShortBase});
    case TypeId::kIntBase:
      return IsOneOf(lhs.base, {TypeId::kByteBase, TypeId::kCharBase, TypeId::kShortBase});
    default: throw; // Should be unreachable.
  }
}

bool TypeChecker::IsReferenceWidening(TypeId lhs, TypeId rhs) const {
  if (!IsReference(lhs) || !IsReference(rhs)) {
    return false;
  }

  // No reference type widens to null.
  if (lhs.base == TypeId::kNullBase) {
    CHECK(lhs.ndims == 0);
    return false;
  }

  // null widens to any reference type.
  if (rhs.base == TypeId::kNullBase) {
    CHECK(rhs.ndims == 0);
    return true;
  }

  // Check if lhs is an ancestor of rhs.
  return typeinfo_.IsAncestor(rhs, lhs);
}

bool TypeChecker::IsAssignable(TypeId lhs, TypeId rhs) const {
  // Identity conversion.
  if (lhs == rhs) {
    return true;
  }

  // If both arrays are of same dimension, recurse on their base types.
  if (lhs.ndims == rhs.ndims && lhs.ndims > 0) {
    // Arrays of primitives can only be assigned to exactly the same types.
    if (IsPrimitive({lhs.base, 0}) || IsPrimitive({rhs.base, 0})) {
      return false;
    }

    return IsAssignable(TypeId{lhs.base, 0}, TypeId{rhs.base, 0});
  }

  // Widening primitive conversion.
  if (IsPrimitiveWidening(lhs, rhs)) {
    return true;
  }

  // Widening reference conversion.
  if (IsReferenceWidening(lhs, rhs)) {
    return true;
  }

  return false;
}

bool TypeChecker::IsCastable(TypeId lhs, TypeId rhs) const {
  if (lhs == rhs) {
    return true;
  }
  if (IsPrimitive(lhs) && IsPrimitive(rhs)) {
    return IsPrimitiveWidening(lhs, rhs) || IsPrimitiveNarrowing(lhs, rhs);
  }
  if (IsReference(lhs) && IsReference(rhs)) {
    return IsAssignable(lhs, rhs) || IsAssignable(rhs, lhs);
  }
  return false;
}

// Returns whether `==' and `!=' would be valid between values of type lhs and
// rhs.
bool TypeChecker::IsComparable(TypeId lhs, TypeId rhs) const {
  // Identical types can be compared.
  if (lhs == rhs) {
    return true;
  }

  // If either are numeric, both must be numeric.
  if (IsNumeric(lhs) && IsNumeric(rhs)) {
    return true;
  }
  if (IsNumeric(lhs) || IsNumeric(rhs)) {
    return false;
  }

  CHECK(IsReference(lhs) && IsReference(rhs));
  const TypeId kNull = TypeId{TypeId::kNullBase, 0};

  if (lhs == kNull || rhs == kNull) {
    return true;
  }

  return IsAssignable(lhs, rhs) || IsAssignable(rhs, lhs);
}

} // namespace types
