#include "base/macros.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "weeder/int_range_visitor.h"

using base::Error;
using base::FileSet;
using lexer::SUB;
using lexer::Token;
using parser::IntLitExpr;

namespace weeder {

namespace {

Error* MakeInvalidIntRangeError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "InvalidIntRangeError",
      "Ints must be between -2^-31 and 2^31 - 1 inclusive.");
}

void VerifyIsInRange(const string& strVal, Token token, bool isNegated,
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
  }
}

}  // namespace

REC_VISIT_DEFN(IntRangeVisitor, IntLitExpr, expr) {
  const string& strVal = expr->Value();
  Token token = expr->GetToken();

  VerifyIsInRange(strVal, token, false, fs_, errors_);
  return false;
}

REC_VISIT_DEFN(IntRangeVisitor, UnaryExpr, expr) {
  if (expr->Op().type == SUB && IS_CONST_PTR(IntLitExpr, expr->Rhs())) {
    const IntLitExpr* intExpr = dynamic_cast<const IntLitExpr*>(expr->Rhs());
    VerifyIsInRange(intExpr->Value(), intExpr->GetToken(), true, fs_, errors_);
    return false;
  }

  return true;
}

}  // namespace weeder
