#include "base/macros.h"
#include "ast/ast.h"
#include "weeder/call_visitor.h"

using ast::Expr;
using ast::FieldDerefExpr;
using ast::NameExpr;
using ast::ThisExpr;
using base::Error;
using base::FileSet;
using lexer::Token;

namespace weeder {

Error* MakeInvalidCallError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "InvalidCallError",
                                 "Cannot call non-method.");
}

Error* MakeExplicitThisCallError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "ExplicitThisCallError",
      "Cannot call explicit 'this' constructor in Joos.");
}

REC_VISIT_DEFN(CallVisitor, CallExpr, expr) {
  const Expr& base = expr.Base();

  if (!IS_CONST_REF(FieldDerefExpr, base) && !IS_CONST_REF(NameExpr, base)) {
    if (IS_CONST_REF(ThisExpr, base)) {
      errors_->Append(MakeExplicitThisCallError(fs_, expr.Lparen()));
    } else {
      errors_->Append(MakeInvalidCallError(fs_, expr.Lparen()));
    }
    return true;
  }

  return true;
}

}  // namespace weeder
