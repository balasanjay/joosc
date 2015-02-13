#include "weeder/int_range_visitor.h"

#include "ast/ast.h"
#include "base/macros.h"
#include "lexer/lexer.h"

using ast::IntLitExpr;
using ast::VisitResult;
using base::Error;
using base::FileSet;
using lexer::SUB;
using lexer::Token;

namespace weeder {

namespace {

Error* MakeInvalidIntRangeError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "InvalidIntRangeError",
      "Ints must be between -2^-31 and 2^31 - 1 inclusive.");
}

bool VerifyIsInRange(const string& strVal, Token token, bool isNegated,
                     const base::FileSet* fs, base::ErrorList* errors) {
  const i64 INT_32_MIN = -(1L << 31L);
  const i64 INT_32_MAX = (1L << 31L) - 1L;
  i64 intVal = 0;

  stringstream ss(strVal);
  ss >> intVal;

  if (isNegated) {
    intVal *= -1;
  }

  if (intVal < INT_32_MIN || intVal > INT_32_MAX || !ss) {
    errors->Append(MakeInvalidIntRangeError(fs, token));
    return false;
  }
  return true;
}

}  // namespace

VISIT_DEFN2(IntRangeVisitor, IntLitExpr, expr) {
  const string& strVal = expr.Value();
  Token token = expr.GetToken();

  if (!VerifyIsInRange(strVal, token, false, fs_, errors_)) {
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN2(IntRangeVisitor, UnaryExpr, expr) {
  if (expr.Op().type != SUB || !IS_CONST_REF(IntLitExpr, expr.Rhs())) {
    return VisitResult::RECURSE;
  }

  const IntLitExpr& intExpr = dynamic_cast<const IntLitExpr&>(expr.Rhs());
  if (!VerifyIsInRange(intExpr.Value(), intExpr.GetToken(), true, fs_, errors_)) {
    return VisitResult::SKIP_PRUNE;
  }

  // We know we're at a base-case, an in-range negative int-literal, so we can
  // skip the recursion. Otherwise, the int-literal might get caught as
  // out-of-range for a positive int-literal.
  return VisitResult::SKIP;
}

}  // namespace weeder
