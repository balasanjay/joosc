#ifndef PARSER_RECURSIVE_VISITOR_H
#define PARSER_RECURSIVE_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/visitor.h"

// TODO: this needs to be moved to weeder.
namespace parser {

// RecursiveVisitor implements all the abstract methods on the Visitor type,
// and performs an exhaustive traversal of the AST. Subclasses that only care
// about inspecting particular nodes can subclass this class, to avoid having
// to write and maintain the traversal code.
//
// Note that instead of directly overriding the Visitor methods, subclasses
// must override special *Impl methods instead. For example, to inspect
// BinExprs, override "bool VisitBinExprImpl(const BinExpr*)". Return true from
// this method to have the RecursiveVisitor continue the traversal as usual. If
// you want to avoid recursing further down the AST, return false.  You can
// also customize the recursion order by doing the recursion 'manually', and
// return false from the Impl method, signalling to the RecursiveVisitor that

class RecursiveVisitor : public Visitor {
public:
  virtual ~RecursiveVisitor() = default;

  // Override Visitor's Expr visitors.
  VISIT(ArrayIndexExpr, expr) final;
  VISIT(BinExpr, expr) final;
  VISIT(CallExpr, expr) final;
  VISIT(CastExpr, expr) final;
  VISIT(FieldDerefExpr, expr) final;
  VISIT(LitExpr, expr) final;
  VISIT(NameExpr, expr) final;
  VISIT(NewArrayExpr, expr) final;
  VISIT(NewClassExpr, expr) final;
  VISIT(ThisExpr,) final;
  VISIT(UnaryExpr, expr) final;

  // Override Visitor's Stmt visitors.
  VISIT(BlockStmt, stmt) final;
  VISIT(EmptyStmt,) final;
  VISIT(ExprStmt, stmt) final;
  VISIT(LocalDeclStmt, stmt) final;
  VISIT(ReturnStmt, stmt) final;
  VISIT(IfStmt, stmt) final;
  VISIT(ForStmt, stmt) final;

  // Override Visitor's other visitors.
  VISIT(ArgumentList, args) final;


#define RECURSIVE_VISITOR_IMPL(type) \
  virtual bool Visit##type##Impl(const type*) { return true; }
  // Declare *Impl methods for Exprs.
  RECURSIVE_VISITOR_IMPL(ArrayIndexExpr);
  RECURSIVE_VISITOR_IMPL(BinExpr);
  RECURSIVE_VISITOR_IMPL(CallExpr);
  RECURSIVE_VISITOR_IMPL(CastExpr);
  RECURSIVE_VISITOR_IMPL(FieldDerefExpr);
  RECURSIVE_VISITOR_IMPL(LitExpr);
  RECURSIVE_VISITOR_IMPL(NameExpr);
  RECURSIVE_VISITOR_IMPL(NewArrayExpr);
  RECURSIVE_VISITOR_IMPL(NewClassExpr);
  RECURSIVE_VISITOR_IMPL(ThisExpr);
  RECURSIVE_VISITOR_IMPL(UnaryExpr);

  // Declare *Impl methods for Stmts.
  RECURSIVE_VISITOR_IMPL(BlockStmt);
  RECURSIVE_VISITOR_IMPL(EmptyStmt);
  RECURSIVE_VISITOR_IMPL(ExprStmt);
  RECURSIVE_VISITOR_IMPL(LocalDeclStmt);
  RECURSIVE_VISITOR_IMPL(ReturnStmt);
  RECURSIVE_VISITOR_IMPL(IfStmt);
  RECURSIVE_VISITOR_IMPL(ForStmt);

  // Declare other *Impl methods.
  RECURSIVE_VISITOR_IMPL(ArgumentList);

#undef RECURSIVE_VISITOR_IMPL

protected:
  RecursiveVisitor() = default;
};

} // namespace parser

#endif
