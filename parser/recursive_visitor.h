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
  virtual ~RecursiveVisitor() {}

  // Override Visitor's Expr visitors.
  VISIT_DECL(ArrayIndexExpr, expr) final;
  VISIT_DECL(BinExpr, expr) final;
  VISIT_DECL(CallExpr, expr) final;
  VISIT_DECL(CastExpr, expr) final;
  VISIT_DECL(FieldDerefExpr, expr) final;
  VISIT_DECL(LitExpr, expr) final;
  VISIT_DECL(NameExpr, expr) final;
  VISIT_DECL(NewArrayExpr, expr) final;
  VISIT_DECL(NewClassExpr, expr) final;
  VISIT_DECL(ThisExpr,) final;
  VISIT_DECL(UnaryExpr, expr) final;

  // Override Visitor's Stmt visitors.
  VISIT_DECL(BlockStmt, stmt) final;
  VISIT_DECL(EmptyStmt,) final;
  VISIT_DECL(ExprStmt, stmt) final;
  VISIT_DECL(LocalDeclStmt, stmt) final;
  VISIT_DECL(ReturnStmt, stmt) final;
  VISIT_DECL(IfStmt, stmt) final;
  VISIT_DECL(ForStmt, stmt) final;

  // Override Visitor's other visitors.
  VISIT_DECL(ArgumentList, args) final;
  VISIT_DECL(ParamList, args) final;
  VISIT_DECL(Param, args) final;
  VISIT_DECL(FieldDecl, args) final;
  VISIT_DECL(MethodDecl, args) final;
  VISIT_DECL(ClassDecl, args) final;
  VISIT_DECL(InterfaceDecl, args) final;


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
  RECURSIVE_VISITOR_IMPL(ParamList);
  RECURSIVE_VISITOR_IMPL(Param);
  RECURSIVE_VISITOR_IMPL(FieldDecl);
  RECURSIVE_VISITOR_IMPL(MethodDecl);
  RECURSIVE_VISITOR_IMPL(ClassDecl);
  RECURSIVE_VISITOR_IMPL(InterfaceDecl);

#undef RECURSIVE_VISITOR_IMPL

protected:
  RecursiveVisitor() = default;
};

#define REC_VISIT_DECL(type, var) \
  bool Visit##type##Impl(const parser::type* var) override
#define REC_VISIT_DEFN(cls, type, var) \
  bool cls::Visit##type##Impl(const parser::type* var)

} // namespace parser

#endif
