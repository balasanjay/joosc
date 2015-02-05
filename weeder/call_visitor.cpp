#include "base/macros.h"
#include "parser/ast.h"
#include "weeder/call_visitor.h"

using base::Error;
using base::FileSet;
using lexer::Token;
using parser::Expr;
using parser::FieldDerefExpr;
using parser::NameExpr;
using parser::ThisExpr;

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
  const Expr* base = expr->Base();

  if (!IS_CONST_PTR(FieldDerefExpr, base) && !IS_CONST_PTR(NameExpr, base)) {
    if (IS_CONST_PTR(ThisExpr, base)) {
      errors_->Append(MakeExplicitThisCallError(fs_, expr->Lparen()));
    } else {
      errors_->Append(MakeInvalidCallError(fs_, expr->Lparen()));
    }
    return true;
  }

  return true;
}

}  // namespace weeder
