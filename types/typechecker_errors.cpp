#include "types/typechecker.h"

#include "ast/ast.h"
#include "base/error.h"
#include "types/types_internal.h"

using ast::TypeId;
using base::Error;
using base::PosRange;

#define N(tid) typeinfo_.LookupTypeName(tid)

namespace types {

Error* TypeChecker::MakeTypeMismatchError(TypeId expected, TypeId got, PosRange pos) {
  stringstream ss;
  ss << "Type mismatch; expected " << N(expected) << ", got " << N(got) << ".";
  return MakeSimplePosRangeError(pos, "TypeMismatchError", ss.str());
}

Error* TypeChecker::MakeIndexNonArrayError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "IndexNonArrayError", "Cannot index non-array.");
}

Error* TypeChecker::MakeIncompatibleCastError(TypeId lhs, TypeId rhs, PosRange pos) {
  stringstream ss;
  ss << "Incompatible types in cast, " << N(lhs) << " and " << N(rhs) << ".";
  return MakeSimplePosRangeError(pos, "IncompatibleCastError", ss.str());
}

Error* TypeChecker::MakeIncompatibleInstanceOfError(TypeId lhs, TypeId rhs, PosRange pos) {
  stringstream ss;
  ss << "Incompatible types in instanceof, " << N(lhs) << " and " << N(rhs) << ".";
  return MakeSimplePosRangeError(pos, "IncompatibleInstanceOfError", ss.str());
}

Error* TypeChecker::MakeInstanceOfPrimitiveError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "InstanceOfPrimitiveError", "Cannot use instanceof with primitive types.");
}

Error* TypeChecker::MakeNoStringError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "NoStringError", "Unknown type java.lang.String.");
}

Error* TypeChecker::MakeUnaryNonNumericError(TypeId rhs, PosRange pos) {
  stringstream ss;
  ss << "Incompatible types in unary expr; expected numeric type, got " << N(rhs) << ".";
  return MakeSimplePosRangeError(pos, "UnaryNonNumericError", ss.str());
}

Error* TypeChecker::MakeUnaryNonBoolError(TypeId rhs, PosRange pos) {
  stringstream ss;
  ss << "Incompatible types in unary expr; expected boolean, got " << N(rhs) << ".";
  return MakeSimplePosRangeError(pos, "UnaryNonBoolError", ss.str());
}

Error* TypeChecker::MakeUnassignableError(TypeId lhs, TypeId rhs, PosRange pos) {
  stringstream ss;
  ss << "Cannot assign " << N(rhs) << " to " << N(lhs) << ".";
  return MakeSimplePosRangeError(pos, "UnassignableError", ss.str());
}

Error* TypeChecker::MakeReturnInVoidMethodError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "ReturnInVoidMethodError", "Cannot return expression in void method or constructor.");
}

Error* TypeChecker::MakeEmptyReturnInNonVoidMethodError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "EmptyReturnInNonVoidMethodError", "Must return expression in non-void method.");
}

Error* TypeChecker::MakeInvalidReturnError(TypeId ret, TypeId expr, PosRange pos) {
  stringstream ss;
  ss << "Cannot return " << N(expr) << " in method returning " << N(ret) << ".";
  return MakeSimplePosRangeError(pos, "InvalidReturnError", ss.str());
}

Error* TypeChecker::MakeIncomparableTypeError(TypeId lhs, TypeId rhs, PosRange pos) {
  stringstream ss;
  ss << "Cannot compare " << N(lhs) << " with " << N(rhs) << ".";
  return MakeSimplePosRangeError(pos, "IncomparableTypeError", ss.str());
}

Error* TypeChecker::MakeThisInStaticMemberError(PosRange this_pos) {
  return MakeSimplePosRangeError(this_pos, "ThisInStaticMemberError", "Cannot use 'this' in static context.");
}

Error* TypeChecker::MakeMemberAccessOnPrimitiveError(TypeId lhs, PosRange pos) {
  stringstream ss;
  ss << "Primitive type " << N(lhs) << " has no members.";
  return MakeSimplePosRangeError(pos, "MemberAccessOnPrimitiveError", ss.str());
}

Error* TypeChecker::MakeTypeInParensError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "TypeInParensError", "Can only put parentheses around a type when casting.");
}

Error* TypeChecker::MakeAssignFinalError(base::PosRange pos) {
  return MakeSimplePosRangeError(pos, "AssignFinalError", "Cannot assign to a final field or variable.");
}

Error* TypeChecker::MakeVoidInExprError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "VoidInExprError", "Expressions returning void cannot be used in this context.");
}

} // namespace types
