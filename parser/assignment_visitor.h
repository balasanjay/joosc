#ifndef PARSER_ASSIGNMENT_VISITOR_H
#define PARSER_ASSIGNMENT_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/visitor.h"

// TODO: this needs to be moved to weeder.
namespace parser {

// AssignmentVisitor checks that the left-hand-side of an assignment is one of
// NameExpr, FieldDerefExpr, or ArrayIndexExpr.
class AssignmentVisitor : public Visitor {
public:
  AssignmentVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  VISIT(ArrayIndexExpr, expr);
  VISIT(BinExpr, expr);
  VISIT(CallExpr, expr);
  VISIT(CastExpr, expr);
  VISIT(FieldDerefExpr, expr);
  VISIT(LitExpr, expr);
  VISIT(NameExpr, expr);
  VISIT(NewArrayExpr, expr);
  VISIT(NewClassExpr, expr);
  VISIT(ThisExpr,);
  VISIT(UnaryExpr, expr);

  VISIT(BlockStmt, stmt);
  VISIT(EmptyStmt,);
  VISIT(ExprStmt, stmt);
  VISIT(LocalDeclStmt, stmt);
  VISIT(ReturnStmt, stmt);
  VISIT(IfStmt, stmt);
  VISIT(ForStmt, stmt);

  VISIT(ArgumentList, args);

private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

} // namespace parser

#endif
