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


#define REWRITE_DECL(type, ret_type, var) \
  virtual ret_type* Rewrite##type(const parser::type& var)

#define REWRITE_DEFN(cls, type, ret_type, var) \
  ret_type* cls::Rewrite##type(const parser::type& var)

// Similar to RecursiveVisitor.
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
  REWRITE_DECL(ImportDecl, ImportDecl, args);
  REWRITE_DECL(CompUnit, CompUnit, args);
  REWRITE_DECL(Program, Program, args);

protected:
  Rewriter() = default;
};

}  // namespace parser

#endif
