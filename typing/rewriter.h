#ifndef TYPING_REWRITER_H
#define TYPING_REWRITER_H

#include "base/fileset.h"
#include "base/errorlist.h"

namespace parser {

class ArrayIndexExpr;
class Expr;
class BinExpr;
class CallExpr;
class CastExpr;
class FieldDerefExpr;
class BoolLitExpr;
class StringLitExpr;
class CharLitExpr;
class IntLitExpr;
class NullLitExpr;
class NameExpr;
class NewArrayExpr;
class NewClassExpr;
class ParenExpr;
class ThisExpr;
class UnaryExpr;
class InstanceOfExpr;

class Stmt;
class BlockStmt;
class EmptyStmt;
class ExprStmt;
class LocalDeclStmt;
class ReturnStmt;
class IfStmt;
class ForStmt;
class WhileStmt;

class ArgumentList;
class ParamList;
class Param;
class MemberDecl;
class FieldDecl;
class MethodDecl;
class ConstructorDecl;
class TypeDecl;
class ClassDecl;
class InterfaceDecl;
class ImportDecl;
class CompUnit;
class Program;


#define REWRITE_VISIT_DECL(type, ret_type, var) \
  virtual ret_type* Visit##type(const parser::type& var)

#define REWRITE_VISIT_DEFN(cls, type, ret_type, var) \
  ret_type* cls::Visit##type(const parser::type& var)

// Similar to RecursiveVisitor.

class Rewriter {
 public:
  virtual ~Rewriter() {}

  REWRITE_VISIT_DECL(ArrayIndexExpr, Expr, expr);
  REWRITE_VISIT_DECL(BinExpr, Expr, expr);
  REWRITE_VISIT_DECL(CallExpr, Expr, expr);
  REWRITE_VISIT_DECL(CastExpr, Expr, expr);
  REWRITE_VISIT_DECL(FieldDerefExpr, Expr, expr);
  REWRITE_VISIT_DECL(BoolLitExpr, Expr, expr);
  REWRITE_VISIT_DECL(StringLitExpr, Expr, expr);
  REWRITE_VISIT_DECL(CharLitExpr, Expr, expr);
  REWRITE_VISIT_DECL(IntLitExpr, Expr, expr);
  REWRITE_VISIT_DECL(NullLitExpr, Expr, expr);
  REWRITE_VISIT_DECL(NameExpr, Expr, expr);
  REWRITE_VISIT_DECL(NewArrayExpr, Expr, expr);
  REWRITE_VISIT_DECL(NewClassExpr, Expr, expr);
  REWRITE_VISIT_DECL(ParenExpr, Expr, expr);
  REWRITE_VISIT_DECL(ThisExpr, Expr, expr);
  REWRITE_VISIT_DECL(UnaryExpr, Expr, expr);
  REWRITE_VISIT_DECL(InstanceOfExpr, Expr, expr);

  // Override Visitor's Stmt visitors.
  REWRITE_VISIT_DECL(BlockStmt, Stmt, stmt);
  REWRITE_VISIT_DECL(EmptyStmt, Stmt, );
  REWRITE_VISIT_DECL(ExprStmt, Stmt, stmt);
  REWRITE_VISIT_DECL(LocalDeclStmt, Stmt, stmt);
  REWRITE_VISIT_DECL(ReturnStmt, Stmt, stmt);
  REWRITE_VISIT_DECL(IfStmt, Stmt, stmt);
  REWRITE_VISIT_DECL(ForStmt, Stmt, stmt);
  REWRITE_VISIT_DECL(WhileStmt, Stmt, stmt);

  // Override Visitor's other visitors.
  REWRITE_VISIT_DECL(ArgumentList, ArgumentList, args);
  REWRITE_VISIT_DECL(ParamList, ParamList, args);
  REWRITE_VISIT_DECL(Param, Param, args);
  REWRITE_VISIT_DECL(FieldDecl, MemberDecl, args);
  REWRITE_VISIT_DECL(MethodDecl, MemberDecl, args);
  REWRITE_VISIT_DECL(ConstructorDecl, MemberDecl, args);
  REWRITE_VISIT_DECL(ClassDecl, TypeDecl, args);
  REWRITE_VISIT_DECL(InterfaceDecl, TypeDecl, args);
  REWRITE_VISIT_DECL(ImportDecl, ImportDecl, args);
  REWRITE_VISIT_DECL(CompUnit, CompUnit, args);
  REWRITE_VISIT_DECL(Program, Program, args);

 //protected:
  Rewriter() = default;
};

}  // namespace typing

#endif
