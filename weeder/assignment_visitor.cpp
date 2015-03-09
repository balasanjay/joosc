#include "base/macros.h"
#include "lexer/lexer.h"
#include "ast/ast.h"
#include "weeder/assignment_visitor.h"

using ast::ArrayIndexExpr;
using ast::BinExpr;
using ast::Expr;
using ast::FieldDerefExpr;
using ast::NameExpr;
using ast::VisitResult;
using base::Error;
using lexer::ASSG;
using lexer::Token;

namespace weeder {

namespace {

Error* MakeInvalidLHSError(Token token) {
  return MakeSimplePosRangeError(token.pos, "InvalidLHSError",
                                 "Invalid left-hand-side of assignment.");
}

} // namespace

VISIT_DEFN(AssignmentVisitor, BinExpr, expr,) {
  if (expr.Op().type != ASSG) {
    return VisitResult::RECURSE;
  }

  const Expr& lhs = expr.Lhs();
  if (!IS_CONST_REF(FieldDerefExpr, lhs) &&
      !IS_CONST_REF(ArrayIndexExpr, lhs) && !IS_CONST_REF(NameExpr, lhs)) {
    errors_->Append(MakeInvalidLHSError(expr.Op()));
    return VisitResult::RECURSE_PRUNE;
  }

  return VisitResult::RECURSE;
}

}  // namespace weeder
