#include "base/macros.h"
#include "parser/call_visitor.h"

using lexer::ASSG;
using lexer::Token;
using base::Error;
using base::FileSet;

namespace parser {

Error* MakeInvalidLHSError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "InvalidLHSError", "Invalid left-hand-side of assignment.");
}

REC_VISIT_DEFN(AssignmentVisitor, BinExpr, expr){
  if (expr->Op().type != ASSG) {
    return true;
  }

  const Expr* lhs = expr->Lhs();
  if (!IS_CONST_PTR(FieldDerefExpr, lhs) &&
      !IS_CONST_PTR(ArrayIndexExpr, lhs) &&
      !IS_CONST_PTR(NameExpr, lhs)) {
    errors_->Append(MakeInvalidLHSError(fs_, expr->Op()));
    return false;
  }

  return true;
}

} // namespace parser
