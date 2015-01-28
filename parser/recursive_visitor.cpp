#include "parser/recursive_visitor.cpp"
#include "parser/ast.h"
#include "parser/print_visitor.h"

using lexer::Token;
using base::Error;
using base::FileSet;

namespace parser {

#define SHORT_CIRCUIT_CHILD(type, var) {\
  if (!Visit##type##impl(var)) { \
    return; \
  } \
}

VISIT_IMPL(RecursiveVisitor, ArrayIndexExpr, expr) {
  SHORT_CIRCUIT_CHILD(ArrayIndexExpr, expr)
  expr->Base()->Accept(this);
  expr->Index()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, BinExpr, expr) {
  SHORT_CIRCUIT_CHILD(BinExpr, expr)
  expr->Lhs()->Accept(this);
  expr->Rhs()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, CallExpr, expr) {
  SHORT_CIRCUIT_CHILD(CallExpr, expr);
  expr->Base()->Accept(this);
  expr->Args()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, CastExpr, expr) {
  SHORT_CIRCUIT_CHILD(CastExpr, expr);
  expr->GetExpr()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, FieldDerefExpr, expr) {
  SHORT_CIRCUIT_CHILD(CastExpr, expr);
  expr->Base()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, LitExpr, expr) {
  SHORT_CIRCUIT_CHILD(LitExpr, expr);
}
VISIT_IMPL(RecursiveVisitor, NameExpr, expr) {
  SHORT_CIRCUIT_CHILD(CastExpr, expr);
}
VISIT_IMPL(RecursiveVisitor, NewArrayExpr, expr) {
  SHORT_CIRCUIT_CHILD(NewArrayExpr, expr);
  if (expr->GetExpr() != nullptr) {
    expr->GetExpr()->Accept(this);
  }
}
VISIT_IMPL(RecursiveVisitor, NewClassExpr, expr) {
  SHORT_CIRCUIT_CHILD(NewClassExpr, expr);
  expr->Args()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, ThisExpr, expr) {}
VISIT_IMPL(RecursiveVisitor, UnaryExpr, expr) {
  expr->Rhs()->Accept(this);
}

VISIT_IMPL(RecursiveVisitor, BlockStmt, stmt) {
  const auto& stmts = stmt->Stmts();
  for (int i = 0; i < stmts.Size(); ++i) {
    stmts.At(i)->Accept(this);
  }
}
VISIT_IMPL(RecursiveVisitor, EmptyStmt,) {}
VISIT_IMPL(RecursiveVisitor, ExprStmt, stmt) {
  stmt->GetExpr()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, LocalDeclStmt, stmt) {
  stmt->GetExpr()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, ReturnStmt, stmt) {
  if (stmt->GetExpr() != nullptr) {
    stmt->GetExpr()->Accept(this);
  }
}
VISIT_IMPL(RecursiveVisitor, IfStmt, stmt) {
  stmt->Cond()->Accept(this);
  stmt->TrueBody()->Accept(this);
  stmt->FalseBody()->Accept(this);
}
VISIT_IMPL(RecursiveVisitor, ForStmt, stmt) {
  stmt->Init()->Accept(this);
  if (stmt->Cond() != nullptr) {
    stmt->Cond()->Accept(this);
  }
  if (stmt->Update() != nullptr) {
    stmt->Update()->Accept(this);
  }
  stmt->Body()->Accept(this);
}

VISIT_IMPL(RecursiveVisitor, ArgumentList, args) {
  for (int i = 0; i < args->Args().Size(); ++i) {
    args->Args().At(i)->Accept(this);
  }
}

} // namespace parser
