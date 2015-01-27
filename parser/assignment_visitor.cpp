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

VISIT_IMPL(AssignmentVisitor, ArrayIndexExpr, expr){
  expr->Base()->Accept(this);
  expr->Index()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, BinExpr, expr){
  if (expr->Op().type != ASSG) {
    expr->Lhs()->Accept(this);
    expr->Rhs()->Accept(this);
    return;
  }

  const Expr* lhs = expr->Lhs();
  if (dynamic_cast<const FieldDerefExpr*>(lhs) == nullptr &&
      dynamic_cast<const ArrayIndexExpr*>(lhs) == nullptr &&
      dynamic_cast<const NameExpr*>(lhs) == nullptr) {
    errors_->Append(MakeInvalidAssignmentLhs(fs_, expr->Op()));
    return;
  }

  expr->Lhs()->Accept(this);
  expr->Rhs()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, CallExpr, expr){
  expr->Base()->Accept(this);
  expr->Args()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, CastExpr, expr){
  expr->GetExpr()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, FieldDerefExpr, expr){
  expr->Base()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, LitExpr,){}
VISIT_IMPL(AssignmentVisitor, NameExpr,){}
VISIT_IMPL(AssignmentVisitor, NewArrayExpr, expr){
  if (expr->GetExpr() != nullptr) {
    expr->GetExpr()->Accept(this);
  }
}
VISIT_IMPL(AssignmentVisitor, NewClassExpr, expr){
  expr->Args()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, ThisExpr,){}
VISIT_IMPL(AssignmentVisitor, UnaryExpr, expr){
  expr->Rhs()->Accept(this);
}

VISIT_IMPL(AssignmentVisitor, BlockStmt, stmt){
  const auto& stmts = stmt->Stmts();
  for (int i = 0; i < stmts.Size(); ++i) {
    stmts.At(i)->Accept(this);
  }
}
VISIT_IMPL(AssignmentVisitor, EmptyStmt,){}
VISIT_IMPL(AssignmentVisitor, ExprStmt, stmt){
  stmt->GetExpr()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, LocalDeclStmt, stmt){
  stmt->GetExpr()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, ReturnStmt, stmt){
  if (stmt->GetExpr() != nullptr) {
    stmt->GetExpr()->Accept(this);
  }
}
VISIT_IMPL(AssignmentVisitor, IfStmt, stmt){
  stmt->Cond()->Accept(this);
  stmt->TrueBody()->Accept(this);
  stmt->FalseBody()->Accept(this);
}
VISIT_IMPL(AssignmentVisitor, ForStmt, stmt){
  stmt->Init()->Accept(this);
  if (stmt->Cond() != nullptr) {
    stmt->Cond()->Accept(this);
  }
  if (stmt->Update() != nullptr) {
    stmt->Update()->Accept(this);
  }
  stmt->Body()->Accept(this);
}

VISIT_IMPL(AssignmentVisitor, ArgumentList, args){
  for (int i = 0; i < args->Args().Size(); ++i) {
    args->Args().At(i)->Accept(this);
  }
}

} // namespace parser
