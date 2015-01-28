#include "parser/assignment_visitor.h"
#include "parser/ast.h"
#include "parser/print_visitor.h"

using lexer::ASSG;
using lexer::Token;
using base::Error;
using base::FileSet;

namespace parser {

Error* MakeInvalidAssignmentLhs(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "InvalidLHS", "Invalid left-hand-side of assignment.");
}

REC_VISIT_DEFN(AssignmentVisitor, BinExpr, expr){
  if (expr->Op().type != ASSG) {
    return true;
  }

  const Expr* lhs = expr->Lhs();
  if (dynamic_cast<const FieldDerefExpr*>(lhs) == nullptr &&
      dynamic_cast<const ArrayIndexExpr*>(lhs) == nullptr &&
      dynamic_cast<const NameExpr*>(lhs) == nullptr) {
    errors_->Append(MakeInvalidAssignmentLhs(fs_, expr->Op()));
    return false;
  }

  return true;
}

} // namespace parser
