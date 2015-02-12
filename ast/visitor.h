#ifndef AST_VISITOR_H
#define AST_VISITOR_H

#include "ast/ast_fwd.h"

namespace ast {

class Visitor {
 public:
  virtual ~Visitor() = default;

#define ABSTRACT_VISIT(type) virtual void Visit##type(const ast::type&) = 0

  ABSTRACT_VISIT(ArrayIndexExpr);
  ABSTRACT_VISIT(BinExpr);
  ABSTRACT_VISIT(CallExpr);
  ABSTRACT_VISIT(CastExpr);
  ABSTRACT_VISIT(FieldDerefExpr);
  ABSTRACT_VISIT(BoolLitExpr);
  ABSTRACT_VISIT(StringLitExpr);
  ABSTRACT_VISIT(CharLitExpr);
  ABSTRACT_VISIT(IntLitExpr);
  ABSTRACT_VISIT(NullLitExpr);
  ABSTRACT_VISIT(NameExpr);
  ABSTRACT_VISIT(NewArrayExpr);
  ABSTRACT_VISIT(NewClassExpr);
  ABSTRACT_VISIT(ThisExpr);
  ABSTRACT_VISIT(ParenExpr);
  ABSTRACT_VISIT(UnaryExpr);
  ABSTRACT_VISIT(InstanceOfExpr);

  ABSTRACT_VISIT(BlockStmt);
  ABSTRACT_VISIT(EmptyStmt);
  ABSTRACT_VISIT(ExprStmt);
  ABSTRACT_VISIT(LocalDeclStmt);
  ABSTRACT_VISIT(ReturnStmt);
  ABSTRACT_VISIT(IfStmt);
  ABSTRACT_VISIT(ForStmt);
  ABSTRACT_VISIT(WhileStmt);

  ABSTRACT_VISIT(Param);
  ABSTRACT_VISIT(ParamList);
  ABSTRACT_VISIT(FieldDecl);
  ABSTRACT_VISIT(MethodDecl);
  ABSTRACT_VISIT(ConstructorDecl);
  ABSTRACT_VISIT(ClassDecl);
  ABSTRACT_VISIT(InterfaceDecl);
  ABSTRACT_VISIT(CompUnit);
  ABSTRACT_VISIT(Program);

#undef ABSTRACT_VISIT

 protected:
  Visitor() = default;
};

#define VISIT_DECL(type, var) void Visit##type(const ast::type& var) override
#define VISIT_DEFN(cls, type, var) \
  void cls::Visit##type(const ast::type& var)

}  // namespace ast

#endif
