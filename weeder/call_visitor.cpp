#include "weeder/call_visitor.h"

#include "ast/ast.h"
#include "base/macros.h"

using ast::Expr;
using ast::FieldDerefExpr;
using ast::NameExpr;
using ast::ThisExpr;
using ast::VisitResult;
using base::Error;
using lexer::Token;

namespace weeder {

namespace {

Error* MakeInvalidCallError(Token token) {
  return MakeSimplePosRangeError(token.pos, "InvalidCallError",
                                 "Cannot call non-method.");
}

Error* MakeExplicitThisCallError(Token token) {
  return MakeSimplePosRangeError(
      token.pos, "ExplicitThisCallError",
      "Cannot call explicit 'this' constructor in Joos.");
}

} // namespace

VISIT_DEFN(CallVisitor, CallExpr, expr,) {
  const Expr& base = expr.Base();

  if (!IS_CONST_REF(FieldDerefExpr, base) && !IS_CONST_REF(NameExpr, base)) {
    if (IS_CONST_REF(ThisExpr, base)) {
      errors_->Append(MakeExplicitThisCallError(expr.Lparen()));
    } else {
      errors_->Append(MakeInvalidCallError(expr.Lparen()));
    }
    return VisitResult::RECURSE_PRUNE;
  }

  return VisitResult::RECURSE;
}

}  // namespace weeder
