#include "types/typechecker.h"

#include "ast/ast.h"
#include "base/error.h"
#include "types/types_internal.h"

using base::Error;
using base::PosRange;

namespace types {

Error* TypeChecker::MakeTypeMismatchError(string expected, string got, PosRange pos) {
  stringstream ss;
  ss << "Type mismatch; expected " << expected << ", got " << got << ".";
  return MakeSimplePosRangeError(pos, "TypeMismatchError", ss.str());
}

Error* TypeChecker::MakeIndexNonArrayError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "IndexNonArrayError", "Cannot index non-array.");
}

Error* TypeChecker::MakeIncompatibleCastError(string lhs, string rhs, PosRange pos) {
  stringstream ss;
  ss << "Incompatible types in cast, " << lhs << " and " << rhs << ".";
  return MakeSimplePosRangeError(pos, "IncompatibleCastError", ss.str());
}

Error* TypeChecker::MakeIncompatibleInstanceOfError(string lhs, string rhs, PosRange pos) {
  stringstream ss;
  ss << "Incompatible types in instanceof, " << lhs << " and " << rhs << ".";
  return MakeSimplePosRangeError(pos, "IncompatibleInstanceOfError", ss.str());
}

Error* TypeChecker::MakeInstanceOfPrimitiveError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "InstanceOfPrimitiveError", "Cannot use instanceof with primitive types.");
}

Error* TypeChecker::MakeNoStringError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "NoStringError", "Unknown type java.lang.String.");
}

Error* TypeChecker::MakeUnaryNonNumericError(string rhs, PosRange pos) {
  stringstream ss;
  ss << "Incompatible types in unary expr; expected numeric type, got " << rhs << ".";
  return MakeSimplePosRangeError(pos, "UnaryNonNumericError", ss.str());
}

Error* TypeChecker::MakeUnaryNonBoolError(string rhs, PosRange pos) {
  stringstream ss;
  ss << "Incompatible types in unary expr; expected boolean, got " << rhs << ".";
  return MakeSimplePosRangeError(pos, "UnaryNonBoolError", ss.str());
}

Error* TypeChecker::MakeUnassignableError(string lhs, string rhs, PosRange pos) {
  stringstream ss;
  ss << "Cannot assign " << rhs << " to " << lhs << ".";
  return MakeSimplePosRangeError(pos, "UnassignableError", ss.str());
}

Error* TypeChecker::MakeInvalidReturnError(string ret, string expr, PosRange pos) {
  if (ret == "void") {
    return MakeSimplePosRangeError(pos, "InvalidReturnError", "Cannot return expression in void method or constructor.");
  }

  stringstream ss;
  ss << "Cannot return " << expr << " in method returning " << ret << ".";
  return MakeSimplePosRangeError(pos, "InvalidReturnError", ss.str());
}

Error* TypeChecker::MakeIncomparableTypeError(string lhs, string rhs, PosRange pos) {
  stringstream ss;
  ss << "Cannot compare " << lhs << " with " << rhs << ".";
  return MakeSimplePosRangeError(pos, "IncomparableTypeError", ss.str());
}

Error* TypeChecker::MakeThisInStaticMemberError(PosRange this_pos) {
  return MakeSimplePosRangeError(this_pos, "ThisInStaticMemberError", "Cannot use 'this' in static context.");
}

} // namespace types
