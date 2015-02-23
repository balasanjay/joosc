#include "types/typechecker.h"

#include "ast/ast.h"
#include "base/error.h"
#include "types/types_internal.h"

using std::initializer_list;

using ast::Type;
using ast::TypeId;
using base::PosRange;

namespace types {

TypeId TypeChecker::MustResolveType(const Type& type) {
  PosRange pos(-1, -1, -1);
  TypeId tid = ResolveType(type, typeset_, &pos);
  if (tid.IsError()) {
    errors_->Append(MakeUnknownTypenameError(fs_, pos));
  }
  return tid;
}

bool TypeChecker::IsNumeric(TypeId tid) const {
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

  return tid.base > TypeId::kIntBase;
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

bool TypeChecker::IsReferenceWidening(TypeId lhs, TypeId rhs) const {
  if (!IsReference(lhs) || !IsReference(rhs)) {
    return false;
  }
  assert(lhs.base != TypeId::kNullBase);

  // null widens to any reference type.
  if (rhs.base == TypeId::kNullBase) {
    assert(rhs.ndims == 0);
    return true;
  }

  // TODO: Handle reference types correctly.
  return false;
}

bool TypeChecker::IsAssignable(TypeId lhs, TypeId rhs) const {
  // Identity conversion.
  if (lhs == rhs) {
    return true;
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

  assert(IsReference(lhs) && IsReference(rhs));
  const TypeId kNull = TypeId{TypeId::kNullBase, 0};

  if (lhs == kNull || rhs == kNull) {
    return true;
  }

  return IsAssignable(lhs, rhs) || IsAssignable(rhs, lhs);
}

} // namespace types
