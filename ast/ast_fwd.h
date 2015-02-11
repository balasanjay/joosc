#ifndef AST_AST_FWD_H
#define AST_AST_FWD_H

namespace ast {

class Expr;
class ArrayIndexExpr;
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
class ClassDecl;
class CompUnit;
class ConstructorDecl;
class FieldDecl;
class InterfaceDecl;
class MemberDecl;
class MethodDecl;
class Param;
class ParamList;
class Program;
class TypeDecl;

} // namespace ast

#endif
