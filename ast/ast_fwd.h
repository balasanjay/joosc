#ifndef AST_AST_FWD_H
#define AST_AST_FWD_H

namespace ast {

class Type;
class ReferenceType;
class ArrayType;
class PrimitiveType;

class Expr;
class ArrayIndexExpr;
class BinExpr;
class BoolLitExpr;
class CallExpr;
class CastExpr;
class CharLitExpr;
class FieldDerefExpr;
class InstanceOfExpr;
class IntLitExpr;
class NameExpr;
class NewArrayExpr;
class NewClassExpr;
class NullLitExpr;
class ParenExpr;
class StaticRefExpr;
class StringLitExpr;
class ThisExpr;
class UnaryExpr;
class FoldedConstantExpr;

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
class CompUnit;
class FieldDecl;
class MemberDecl;
class MethodDecl;
class TypeDecl;
class Param;
class ParamList;
class Program;

} // namespace ast

#endif
