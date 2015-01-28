#include "parser/recursive_visitor.h"
#include "parser/ast.h"
#include "parser/print_visitor.h"

using lexer::Token;
using base::Error;
using base::FileSet;

namespace parser {

#define SHORT_CIRCUIT_CHILD(type, var) {\
  if (!Visit##type##Impl(var)) { \
    return; \
  } \
}

VISIT_DEFN(RecursiveVisitor, ArrayIndexExpr, expr) {
  SHORT_CIRCUIT_CHILD(ArrayIndexExpr, expr)
  expr->Base()->Accept(this);
  expr->Index()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, BinExpr, expr) {
  SHORT_CIRCUIT_CHILD(BinExpr, expr)
  expr->Lhs()->Accept(this);
  expr->Rhs()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, CallExpr, expr) {
  SHORT_CIRCUIT_CHILD(CallExpr, expr);
  expr->Base()->Accept(this);
  expr->Args()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, CastExpr, expr) {
  SHORT_CIRCUIT_CHILD(CastExpr, expr);
  expr->GetExpr()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, FieldDerefExpr, expr) {
  SHORT_CIRCUIT_CHILD(FieldDerefExpr, expr);
  expr->Base()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, LitExpr, expr) {
  SHORT_CIRCUIT_CHILD(LitExpr, expr);
}
VISIT_DEFN(RecursiveVisitor, NameExpr, expr) {
  SHORT_CIRCUIT_CHILD(CastExpr, expr);
}
VISIT_DEFN(RecursiveVisitor, NewArrayExpr, expr) {
  SHORT_CIRCUIT_CHILD(NewArrayExpr, expr);
  if (expr->GetExpr() != nullptr) {
    expr->GetExpr()->Accept(this);
  }
}
VISIT_DEFN(RecursiveVisitor, NewClassExpr, expr) {
  SHORT_CIRCUIT_CHILD(NewClassExpr, expr);
  expr->Args()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, ThisExpr, expr) {
  SHORT_CIRCUIT_CHILD(ThisExpr, expr);
}
VISIT_DEFN(RecursiveVisitor, UnaryExpr, expr) {
  SHORT_CIRCUIT_CHILD(UnaryExpr, expr);
  expr->Rhs()->Accept(this);
}

VISIT_DEFN(RecursiveVisitor, BlockStmt, stmt) {
  SHORT_CIRCUIT_CHILD(BlockStmt, stmt);
  const auto& stmts = stmt->Stmts();
  for (int i = 0; i < stmts.Size(); ++i) {
    stmts.At(i)->Accept(this);
  }
}
VISIT_DEFN(RecursiveVisitor, EmptyStmt, stmt) {
  SHORT_CIRCUIT_CHILD(EmptyStmt, stmt);
}
VISIT_DEFN(RecursiveVisitor, ExprStmt, stmt) {
  SHORT_CIRCUIT_CHILD(ExprStmt, stmt);
  stmt->GetExpr()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, LocalDeclStmt, stmt) {
  SHORT_CIRCUIT_CHILD(LocalDeclStmt, stmt);
  stmt->GetExpr()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, ReturnStmt, stmt) {
  SHORT_CIRCUIT_CHILD(ReturnStmt, stmt);
  if (stmt->GetExpr() != nullptr) {
    stmt->GetExpr()->Accept(this);
  }
}
VISIT_DEFN(RecursiveVisitor, IfStmt, stmt) {
  SHORT_CIRCUIT_CHILD(IfStmt, stmt);
  stmt->Cond()->Accept(this);
  stmt->TrueBody()->Accept(this);
  stmt->FalseBody()->Accept(this);
}
VISIT_DEFN(RecursiveVisitor, ForStmt, stmt) {
  SHORT_CIRCUIT_CHILD(ForStmt, stmt);
  stmt->Init()->Accept(this);
  if (stmt->Cond() != nullptr) {
    stmt->Cond()->Accept(this);
  }
  if (stmt->Update() != nullptr) {
    stmt->Update()->Accept(this);
  }
  stmt->Body()->Accept(this);
}

VISIT_DEFN(RecursiveVisitor, ArgumentList, args) {
  SHORT_CIRCUIT_CHILD(ArgumentList, args);
  for (int i = 0; i < args->Args().Size(); ++i) {
    args->Args().At(i)->Accept(this);
  }
}

} // namespace parser
