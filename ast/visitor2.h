#ifndef AST_VISITOR2_H
#define AST_VISITOR2_H

#include "ast/ast_fwd.h"

namespace ast {

#define FOR_EACH_VISITABLE(code) \
  code(ArrayIndexExpr, Expr, expr) \
  code(BinExpr, Expr, expr) \
  code(CallExpr, Expr, expr) \
  code(CastExpr, Expr, expr) \
  code(FieldDerefExpr, Expr, expr) \
  code(BoolLitExpr, Expr, expr) \
  code(StringLitExpr, Expr, expr) \
  code(CharLitExpr, Expr, expr) \
  code(IntLitExpr, Expr, expr) \
  code(NullLitExpr, Expr, expr) \
  code(NameExpr, Expr, expr) \
  code(NewArrayExpr, Expr, expr) \
  code(NewClassExpr, Expr, expr) \
  code(ParenExpr, Expr, expr) \
  code(ThisExpr, Expr, expr) \
  code(UnaryExpr, Expr, expr) \
  code(InstanceOfExpr, Expr, expr)

/*
  code(BlockStmt, Stmt, stmt) \
  code(EmptyStmt, Stmt, stmt) \
  code(ExprStmt, Stmt, stmt) \
  code(LocalDeclStmt, Stmt, stmt) \
  code(ReturnStmt, Stmt, stmt) \
  code(IfStmt, Stmt, stmt) \
  code(ForStmt, Stmt, stmt) \
  code(WhileStmt, Stmt, stmt) \
  code(ArgumentList, ArgumentList, args) \
  code(ParamList, ParamList, params) \
  code(Param, Param, param) \
  code(FieldDecl, MemberDecl, field) \
  code(MethodDecl, MemberDecl, meth) \
  code(ConstructorDecl, MemberDecl, cons) \
  code(ClassDecl, TypeDecl, decl) \
  code(InterfaceDecl, TypeDecl, decl) \
  code(CompUnit, CompUnit, unit) \
  code(Program, Program, prog)
*/

/*
class Visitor2 {
public:
  virtual bool VisitArrayIndexExpr(const ArrayIndexExpr& expr) {
    return true;
  }

  virtual sptr<Expr> RewriteArrayIndexExpr(const ArrayIndexExpr& expr, sptr<ArrayIndexExpr> exprptr) {
    if (!VisitArrayIndexExpr(expr)) {
      return exprptr;
    }

    sptr<Expr> base = AcceptExpr(expr.BasePtr());
    sptr<Expr> index = AcceptExpr(expr.IndexPtr());

    if (lhs == nullptr || rhs == nullptr) {
      return nullptr;
    }
    if (base == expr.BasePtr() && index == rhs.IndexPtr()) {
      return exprptr;
    }
    return make_shared<ArrayIndexExpr>(base, index);
  }

#define VISIT_DEFN(type, parent, name) sptr<parent> Visit##type(sptr<type>);
  FOR_EACH_VISITABLE(VISIT_DEFN);
#undef VISIT_DEFN
};
*/

#undef FOR_EACH_VISITABLE

} // namespace ast

#endif
