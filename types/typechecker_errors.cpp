#include "types/typechecker.h"

#include "ast/ast.h"
#include "base/error.h"
#include "types/types_internal.h"

using ast::TypeId;
using base::Error;
using base::PosRange;

namespace types {

Error* TypeChecker::MakeTypeMismatchError(TypeId expected, TypeId got, PosRange pos) {
  // TODO: lookup expected and got in typeinfo_ and get a name.
  stringstream ss;
  ss << "Type mismatch; expected " << expected.base << ", got " << got.base << ".";
  return MakeSimplePosRangeError(fs_, pos, "TypeMismatchError", ss.str());
}

Error* TypeChecker::MakeIndexNonArrayError(PosRange pos) {
  return MakeSimplePosRangeError(fs_, pos, "IndexNonArrayError", "Cannot index non-array.");
}

Error* TypeChecker::MakeIncompatibleCastError(TypeId lhs, TypeId rhs, PosRange pos) {
  // TODO: lookup lhs and rhs in typeinfo_ and get a name.
  stringstream ss;
  ss << "Incompatible types in cast, " << lhs.base << " and " << rhs.base << ".";
  return MakeSimplePosRangeError(fs_, pos, "IncompatibleCastError", ss.str());
}

Error* TypeChecker::MakeIncompatibleInstanceOfError(TypeId lhs, TypeId rhs, PosRange pos) {
  // TODO: lookup lhs and rhs in typeinfo_ and get a name.
  stringstream ss;
  ss << "Incompatible types in instanceof, " << lhs.base << " and " << rhs.base << ".";
  return MakeSimplePosRangeError(fs_, pos, "IncompatibleInstanceOfError", ss.str());
}

Error* TypeChecker::MakeInstanceOfPrimitiveError(PosRange pos) {
  return MakeSimplePosRangeError(fs_, pos, "InstanceOfPrimitiveError", "Cannot use instanceof with primitive types.");
}

Error* TypeChecker::MakeNoStringError(PosRange pos) {
  return MakeSimplePosRangeError(fs_, pos, "NoStringError", "Unknown type java.lang.String.");
}

Error* TypeChecker::MakeUnaryNonNumericError(TypeId rhs, PosRange pos) {
  // TODO: lookup rhs in typeinfo_ and get a name.
  stringstream ss;
  ss << "Incompatible types in unary expr; expected numeric type, got " << rhs.base << ".";
  return MakeSimplePosRangeError(fs_, pos, "UnaryNonNumericError", ss.str());
}

Error* TypeChecker::MakeUnaryNonBoolError(TypeId rhs, PosRange pos) {
  // TODO: lookup rhs in typeinfo_ and get a name.
  stringstream ss;
  ss << "Incompatible types in unary expr; expected boolean, got " << rhs.base << ".";
  return MakeSimplePosRangeError(fs_, pos, "UnaryNonBoolError", ss.str());
}

Error* TypeChecker::MakeUnassignableError(TypeId lhs, TypeId rhs, PosRange pos) {
  // TODO: lookup lhs and rhs in typeinfo_ and get a name.
  stringstream ss;
  ss << "Cannot assign " << rhs.base << " to " << lhs.base << ".";
  return MakeSimplePosRangeError(fs_, pos, "UnassignableError", ss.str());
}

Error* TypeChecker::MakeInvalidReturnError(TypeId ret, TypeId expr, PosRange pos) {
  if (ret.base == TypeId::kVoidBase) {
    return MakeSimplePosRangeError(fs_, pos, "InvalidReturnError", "Cannot return expression in void method or constructor.");
  }

  // TODO: lookup ret and expr in typeinfo_ and get a name.
  stringstream ss;
  ss << "Cannot return " << expr.base << " in method returning " << ret.base << ".";
  return MakeSimplePosRangeError(fs_, pos, "InvalidReturnError", ss.str());
}

Error* TypeChecker::MakeIncomparableTypeError(TypeId lhs, TypeId rhs, PosRange pos) {
  // TODO: lookup lhs and rhs in typeinfo_ and get a name.
  stringstream ss;
  ss << "Cannot compare " << lhs.base << " with " << rhs.base << ".";
  return MakeSimplePosRangeError(fs_, pos, "IncomparableTypeError", ss.str());
}

} // namespace types
