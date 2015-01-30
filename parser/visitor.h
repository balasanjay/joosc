#ifndef PARSER_VISITOR_H
#define PARSER_VISITOR_H

namespace parser {

class ArrayIndexExpr;
class BinExpr;
class CallExpr;
class CastExpr;
class FieldDerefExpr;
class LitExpr;
class NameExpr;
class NewArrayExpr;
class NewClassExpr;
class ThisExpr;
class UnaryExpr;
class InstanceOfExpr;

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
class FieldDecl;
class MethodDecl;
class ConstructorDecl;
class ClassDecl;
class InterfaceDecl;
class ImportDecl;
class CompUnit;
class Program;

class Visitor {
public:
  virtual ~Visitor() = default;

#define ABSTRACT_VISIT(type) virtual void Visit##type(const type*) = 0

  ABSTRACT_VISIT(ArrayIndexExpr);
  ABSTRACT_VISIT(BinExpr);
  ABSTRACT_VISIT(CallExpr);
  ABSTRACT_VISIT(CastExpr);
  ABSTRACT_VISIT(FieldDerefExpr);
  ABSTRACT_VISIT(LitExpr);
  ABSTRACT_VISIT(NameExpr);
  ABSTRACT_VISIT(NewArrayExpr);
  ABSTRACT_VISIT(NewClassExpr);
  ABSTRACT_VISIT(ThisExpr);
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

  ABSTRACT_VISIT(ArgumentList);
  ABSTRACT_VISIT(Param);
  ABSTRACT_VISIT(ParamList);
  ABSTRACT_VISIT(FieldDecl);
  ABSTRACT_VISIT(MethodDecl);
  ABSTRACT_VISIT(ConstructorDecl);
  ABSTRACT_VISIT(ClassDecl);
  ABSTRACT_VISIT(InterfaceDecl);
  ABSTRACT_VISIT(ImportDecl);
  ABSTRACT_VISIT(CompUnit);
  ABSTRACT_VISIT(Program);

#undef ABSTRACT_VISIT

protected:
  Visitor() = default;
};

#define VISIT_DECL(type, var) \
  void Visit##type(const parser::type* var) override
#define VISIT_DEFN(cls, type, var) \
  void cls::Visit##type(const parser::type* var)

} // namespace parser

#endif
