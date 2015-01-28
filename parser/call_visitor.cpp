#include "base/macros.h"
#include "parser/assignment_visitor.h"

using lexer::Token;
using base::Error;
using base::FileSet;

namespace parser {

Error* MakeInvalidCallError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "InvalidCallError", "Cannot call non-method.");
}

REC_VISIT_DEFN(AssignmentVisitor, CallExpr, expr){
  const Expr* base = expr->Base();

  if (!IS_CONST_PTR(FieldDerefExpr, base) &&
      !IS_CONST_PTR(NameExpr, base)) {
    errors_->Append(MakeInvalidCallError(fs_, expr->Lparen()));
    return true;
  }

  return true;
}

} // namespace parser
