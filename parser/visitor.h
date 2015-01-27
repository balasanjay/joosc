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

class BlockStmt;
class EmptyStmt;
class ExprStmt;
class LocalDeclStmt;
class ReturnStmt;
class IfStmt;
class ForStmt;

class ArgumentList;

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

  ABSTRACT_VISIT(BlockStmt);
  ABSTRACT_VISIT(EmptyStmt);
  ABSTRACT_VISIT(ExprStmt);
  ABSTRACT_VISIT(LocalDeclStmt);
  ABSTRACT_VISIT(ReturnStmt);
  ABSTRACT_VISIT(IfStmt);
  ABSTRACT_VISIT(ForStmt);

  ABSTRACT_VISIT(ArgumentList);

#undef ABSTRACT_VISIT

protected:
  Visitor() = default;
};

#define VISIT(type, var) void Visit##type(const type* var) override

} // namespace parser

#endif
