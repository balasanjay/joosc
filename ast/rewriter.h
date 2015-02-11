#ifndef TYPING_REWRITER_H
#define TYPING_REWRITER_H

#include "ast/ast_fwd.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace ast {

#define REWRITE_DECL(type, ret_type, var) \
  virtual ast::ret_type* Rewrite##type(const ast::type& var)

// Rewriter rewrites an AST. All Rewrite* methods perform a deep-clone by
// default; override a method to perform custom rewrites for that corresponding
// AST node.
class Rewriter {
 public:
  virtual ~Rewriter() = default;

  REWRITE_DECL(ArrayIndexExpr, Expr, expr);
  REWRITE_DECL(BinExpr, Expr, expr);
  REWRITE_DECL(CallExpr, Expr, expr);
  REWRITE_DECL(CastExpr, Expr, expr);
  REWRITE_DECL(FieldDerefExpr, Expr, expr);
  REWRITE_DECL(BoolLitExpr, Expr, expr);
  REWRITE_DECL(StringLitExpr, Expr, expr);
  REWRITE_DECL(CharLitExpr, Expr, expr);
  REWRITE_DECL(IntLitExpr, Expr, expr);
  REWRITE_DECL(NullLitExpr, Expr, expr);
  REWRITE_DECL(NameExpr, Expr, expr);
  REWRITE_DECL(NewArrayExpr, Expr, expr);
  REWRITE_DECL(NewClassExpr, Expr, expr);
  REWRITE_DECL(ParenExpr, Expr, expr);
  REWRITE_DECL(ThisExpr, Expr, expr);
  REWRITE_DECL(UnaryExpr, Expr, expr);
  REWRITE_DECL(InstanceOfExpr, Expr, expr);

  REWRITE_DECL(BlockStmt, Stmt, stmt);
  REWRITE_DECL(EmptyStmt, Stmt, );
  REWRITE_DECL(ExprStmt, Stmt, stmt);
  REWRITE_DECL(LocalDeclStmt, Stmt, stmt);
  REWRITE_DECL(ReturnStmt, Stmt, stmt);
  REWRITE_DECL(IfStmt, Stmt, stmt);
  REWRITE_DECL(ForStmt, Stmt, stmt);
  REWRITE_DECL(WhileStmt, Stmt, stmt);

  REWRITE_DECL(ArgumentList, ArgumentList, args);
  REWRITE_DECL(ParamList, ParamList, args);
  REWRITE_DECL(Param, Param, args);
  REWRITE_DECL(FieldDecl, MemberDecl, args);
  REWRITE_DECL(MethodDecl, MemberDecl, args);
  REWRITE_DECL(ConstructorDecl, MemberDecl, args);
  REWRITE_DECL(ClassDecl, TypeDecl, args);
  REWRITE_DECL(InterfaceDecl, TypeDecl, args);
  REWRITE_DECL(CompUnit, CompUnit, args);
  REWRITE_DECL(Program, Program, args);

protected:
  Rewriter() = default;
};

#undef REWRITE_DECL

#define REWRITE_DECL(type, ret_type, var) \
  ast::ret_type* Rewrite##type(const ast::type& var) override

#define REWRITE_DEFN(cls, type, ret_type, var) \
  ast::ret_type* cls::Rewrite##type(const ast::type& var)

}  // namespace ast

#endif
