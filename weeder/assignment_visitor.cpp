#include "base/macros.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "weeder/assignment_visitor.h"

using base::Error;
using base::FileSet;
using lexer::ASSG;
using lexer::Token;
using parser::ArrayIndexExpr;
using parser::BinExpr;
using parser::Expr;
using parser::FieldDerefExpr;
using parser::NameExpr;

namespace weeder {

Error* MakeInvalidLHSError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "InvalidLHSError",
                                 "Invalid left-hand-side of assignment.");
}

REC_VISIT_DEFN(AssignmentVisitor, BinExpr, expr) {
  if (expr.Op().type != ASSG) {
    return true;
  }

  const Expr& lhs = expr.Lhs();
  if (!IS_CONST_REF(FieldDerefExpr, lhs) &&
      !IS_CONST_REF(ArrayIndexExpr, lhs) && !IS_CONST_REF(NameExpr, lhs)) {
    errors_->Append(MakeInvalidLHSError(fs_, expr.Op()));
    return false;
  }

  return true;
}

}  // namespace weeder
