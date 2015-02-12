#ifndef AST_AST_H
#define AST_AST_H

#include <iostream>

#include "ast/ids.h"
#include "ast/rewriter.h"
#include "ast/visitor.h"
#include "lexer/lexer.h"

namespace ast {

#define ACCEPT_VISITOR_ABSTRACT(type) \
  virtual void AcceptVisitor(Visitor* visitor) const = 0; \
  virtual type* AcceptRewriter(Rewriter* visitor) const = 0

#define ACCEPT_VISITOR(type, ret_type) \
  virtual void AcceptVisitor(Visitor* visitor) const { visitor->Visit##type(*this); } \
  virtual ret_type* AcceptRewriter(Rewriter* visitor) const { return visitor->Rewrite##type(*this); }

#define REF_GETTER(type, name, expr) \
  const type& name() const { return (expr); }

#define VAL_GETTER(type, name, expr) \
  type name() const { return (expr); }

class QualifiedName final {
 public:
  QualifiedName(const vector<lexer::Token>& tokens, const vector<string>& parts,
                const string& name)
      : tokens_(tokens), parts_(parts), name_(name) {}

  QualifiedName() = default;
  QualifiedName(const QualifiedName&) = default;
  QualifiedName& operator=(const QualifiedName&) = default;

  void PrintTo(std::ostream* os) const { *os << name_; }

  REF_GETTER(string, Name, name_);
  REF_GETTER(vector<string>, Parts, parts_);
  REF_GETTER(vector<lexer::Token>, Tokens, tokens_);

 private:
  vector<lexer::Token>
      tokens_;            // [IDENTIFIER, DOT, IDENTIFIER, DOT, IDENTIFIER]
  vector<string> parts_;  // ["java", "lang", "String"]
  string name_;           // "java.lang.String"
};

class Type {
 public:
  virtual ~Type() = default;

  virtual void PrintTo(std::ostream* os) const = 0;

  virtual Type* Clone() const = 0;

 protected:
  Type() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(Type);
};

class PrimitiveType : public Type {
 public:
  PrimitiveType(lexer::Token token) : token_(token) {}

  void PrintTo(std::ostream* os) const override { *os << token_.TypeInfo(); }

  VAL_GETTER(lexer::Token, GetToken, token_);

  PrimitiveType* Clone() const override {
    return new PrimitiveType(token_);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PrimitiveType);

  lexer::Token token_;
};

class ReferenceType : public Type {
 public:
  ReferenceType(const QualifiedName& name) : name_(name) {}

  void PrintTo(std::ostream* os) const override { name_.PrintTo(os); }

  REF_GETTER(QualifiedName, Name, name_);

  ReferenceType* Clone() const override {
    return new ReferenceType(name_);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ReferenceType);

  QualifiedName name_;
};

class ArrayType : public Type {
 public:
  ArrayType(Type* elemtype) : elemtype_(elemtype) {}

  void PrintTo(std::ostream* os) const override {
    *os << "array<";
    elemtype_->PrintTo(os);
    *os << '>';
  }

  REF_GETTER(Type, ElemType, *elemtype_);

  Type* Clone() const override {
    return new ArrayType(elemtype_->Clone());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ArrayType);

  uptr<Type> elemtype_;
};

class Expr {
 public:
  virtual ~Expr() = default;

  ACCEPT_VISITOR_ABSTRACT(Expr);

 protected:
  Expr() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(Expr);
};

class ArgumentList final {
 public:
  ArgumentList(base::UniquePtrVector<Expr>&& args)
      : args_(std::forward<base::UniquePtrVector<Expr>>(args)) {}
  ~ArgumentList() = default;
  ArgumentList(ArgumentList&&) = default;

  ACCEPT_VISITOR(ArgumentList, ArgumentList);

  REF_GETTER(base::UniquePtrVector<Expr>, Args, args_);

 private:
  DISALLOW_COPY_AND_ASSIGN(ArgumentList);

  base::UniquePtrVector<Expr> args_;
};

class NameExpr : public Expr {
 public:
  NameExpr(const QualifiedName& name) : name_(name) {}

  ACCEPT_VISITOR(NameExpr, Expr);

  REF_GETTER(QualifiedName, Name, name_);

 private:
  DISALLOW_COPY_AND_ASSIGN(NameExpr);

  QualifiedName name_;
};

class InstanceOfExpr : public Expr {
 public:
  InstanceOfExpr(Expr* lhs, lexer::Token instanceof, Type* type)
      : lhs_(lhs), instanceof_(instanceof), type_(type) {}

  ACCEPT_VISITOR(InstanceOfExpr, Expr);

  REF_GETTER(Expr, Lhs, *lhs_);
  VAL_GETTER(lexer::Token, InstanceOf, instanceof_);
  REF_GETTER(Type, GetType, *type_);

 private:
  DISALLOW_COPY_AND_ASSIGN(InstanceOfExpr);

  uptr<Expr> lhs_;
  lexer::Token instanceof_;
  uptr<Type> type_;
};

class ParenExpr : public Expr {
 public:
  ParenExpr(Expr* nested) : nested_(nested) { assert(nested_ != nullptr); }

  ACCEPT_VISITOR(ParenExpr, Expr);

  REF_GETTER(Expr, Nested, *nested_);

 private:
  uptr<Expr> nested_;
};

class BinExpr : public Expr {
 public:
  BinExpr(Expr* lhs, lexer::Token op, Expr* rhs)
      : op_(op), lhs_(lhs), rhs_(rhs) {
    assert(lhs != nullptr);
    assert(op.TypeInfo().IsBinOp());
    assert(rhs != nullptr);
  }

  ACCEPT_VISITOR(BinExpr, Expr);

  VAL_GETTER(lexer::Token, Op, op_);
  REF_GETTER(Expr, Lhs, *lhs_);
  REF_GETTER(Expr, Rhs, *rhs_);

 private:
  lexer::Token op_;
  uptr<Expr> lhs_;
  uptr<Expr> rhs_;
};

class UnaryExpr : public Expr {
 public:
  UnaryExpr(lexer::Token op, Expr* rhs) : op_(op), rhs_(rhs) {
    assert(op.TypeInfo().IsUnaryOp());
    assert(rhs != nullptr);
  }

  ACCEPT_VISITOR(UnaryExpr, Expr);

  VAL_GETTER(lexer::Token, Op, op_);
  REF_GETTER(Expr, Rhs, *rhs_);

 private:
  lexer::Token op_;
  uptr<Expr> rhs_;
};

class LitExpr : public Expr {
 public:
  LitExpr(lexer::Token token) : token_(token) {}

  VAL_GETTER(lexer::Token, GetToken, token_);

 private:
  lexer::Token token_;
};

class BoolLitExpr : public LitExpr {
 public:
  BoolLitExpr(lexer::Token token) : LitExpr(token) {}

  ACCEPT_VISITOR(BoolLitExpr, Expr);
};

class IntLitExpr : public LitExpr {
 public:
  IntLitExpr(lexer::Token token, const string& value)
      : LitExpr(token), value_(value) {}

  ACCEPT_VISITOR(IntLitExpr, Expr);

  REF_GETTER(string, Value, value_);

 private:
  string value_;
};

class StringLitExpr : public LitExpr {
 public:
  StringLitExpr(lexer::Token token) : LitExpr(token) {}

  ACCEPT_VISITOR(StringLitExpr, Expr);
};

class CharLitExpr : public LitExpr {
 public:
  CharLitExpr(lexer::Token token) : LitExpr(token) {}

  ACCEPT_VISITOR(CharLitExpr, Expr);
};

class NullLitExpr : public LitExpr {
 public:
  NullLitExpr(lexer::Token token) : LitExpr(token) {}

  ACCEPT_VISITOR(NullLitExpr, Expr);
};

class ThisExpr : public Expr {
 public:
  ACCEPT_VISITOR(ThisExpr, Expr);
};

class ArrayIndexExpr : public Expr {
 public:
  ArrayIndexExpr(Expr* base, Expr* index) : base_(base), index_(index) {}

  ACCEPT_VISITOR(ArrayIndexExpr, Expr);

  REF_GETTER(Expr, Base, *base_);
  REF_GETTER(Expr, Index, *index_);

 private:
  uptr<Expr> base_;
  uptr<Expr> index_;
};

class FieldDerefExpr : public Expr {
 public:
  FieldDerefExpr(Expr* base, const string& fieldname, lexer::Token token)
      : base_(base), fieldname_(fieldname), token_(token) {}

  ACCEPT_VISITOR(FieldDerefExpr, Expr);

  REF_GETTER(Expr, Base, *base_);
  REF_GETTER(string, FieldName, fieldname_);
  REF_GETTER(lexer::Token, GetToken, token_);

 private:
  uptr<Expr> base_;
  string fieldname_;
  lexer::Token token_;
};

class CallExpr : public Expr {
 public:
  CallExpr(Expr* base, lexer::Token lparen, ArgumentList&& args)
      : base_(base), lparen_(lparen), args_(std::forward<ArgumentList>(args)) {}

  ACCEPT_VISITOR(CallExpr, Expr);

  REF_GETTER(Expr, Base, *base_);
  VAL_GETTER(lexer::Token, Lparen, lparen_);
  REF_GETTER(ArgumentList, Args, args_);

 private:
  uptr<Expr> base_;
  lexer::Token lparen_;
  ArgumentList args_;
};

class CastExpr : public Expr {
 public:
  CastExpr(Type* type, Expr* expr) : type_(type), expr_(expr) {}

  ACCEPT_VISITOR(CastExpr, Expr);

  REF_GETTER(Type, GetType, *type_);
  REF_GETTER(Expr, GetExpr, *expr_);

 private:
  DISALLOW_COPY_AND_ASSIGN(CastExpr);

  uptr<Type> type_;
  uptr<Expr> expr_;
};

class NewClassExpr : public Expr {
 public:
  NewClassExpr(lexer::Token newTok, Type* type, ArgumentList&& args)
      : newTok_(newTok), type_(type), args_(std::forward<ArgumentList>(args)) {}

  ACCEPT_VISITOR(NewClassExpr, Expr);

  VAL_GETTER(lexer::Token, NewToken, newTok_);
  REF_GETTER(Type, GetType, *type_);
  REF_GETTER(ArgumentList, Args, args_);

 private:
  DISALLOW_COPY_AND_ASSIGN(NewClassExpr);

  lexer::Token newTok_;
  uptr<Type> type_;
  ArgumentList args_;
};

class NewArrayExpr : public Expr {
 public:
  NewArrayExpr(Type* type, Expr* expr) : type_(type), expr_(expr) {}

  ACCEPT_VISITOR(NewArrayExpr, Expr);

  REF_GETTER(Type, GetType, *type_);
  VAL_GETTER(const Expr*, GetExpr, expr_.get());

 private:
  DISALLOW_COPY_AND_ASSIGN(NewArrayExpr);

  uptr<Type> type_;
  uptr<Expr> expr_; // Can be nullptr.
};

class Stmt {
 public:
  virtual ~Stmt() = default;

  ACCEPT_VISITOR_ABSTRACT(Stmt);

 protected:
  Stmt() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(Stmt);
};

class EmptyStmt : public Stmt {
 public:
  EmptyStmt() = default;

  ACCEPT_VISITOR(EmptyStmt, Stmt);
};

class LocalDeclStmt : public Stmt {
 public:
  LocalDeclStmt(Type* type, lexer::Token ident, Expr* expr)
      : type_(type), ident_(ident), expr_(expr) {}

  ACCEPT_VISITOR(LocalDeclStmt, Stmt);

  REF_GETTER(Type, GetType, *type_);
  VAL_GETTER(lexer::Token, Ident, ident_);
  REF_GETTER(Expr, GetExpr, *expr_);

  // TODO: get the identifier as a string.

 private:
  uptr<Type> type_;
  lexer::Token ident_;
  uptr<Expr> expr_;
};

class ReturnStmt : public Stmt {
 public:
  ReturnStmt(Expr* expr) : expr_(expr) {}

  ACCEPT_VISITOR(ReturnStmt, Stmt);

  VAL_GETTER(const Expr*, GetExpr, expr_.get());

 private:
  uptr<Expr> expr_; // Can be nullptr.
};

class ExprStmt : public Stmt {
 public:
  ExprStmt(Expr* expr) : expr_(expr) {}

  ACCEPT_VISITOR(ExprStmt, Stmt);

  REF_GETTER(Expr, GetExpr, *expr_);

 private:
  uptr<Expr> expr_;
};

class BlockStmt : public Stmt {
 public:
  BlockStmt(base::UniquePtrVector<Stmt>&& stmts)
      : stmts_(std::forward<base::UniquePtrVector<Stmt>>(stmts)) {}

  ACCEPT_VISITOR(BlockStmt, Stmt);

  REF_GETTER(base::UniquePtrVector<Stmt>, Stmts, stmts_);

 private:
  DISALLOW_COPY_AND_ASSIGN(BlockStmt);
  base::UniquePtrVector<Stmt> stmts_;
};

class IfStmt : public Stmt {
 public:
  IfStmt(Expr* cond, Stmt* trueBody, Stmt* falseBody)
      : cond_(cond), trueBody_(trueBody), falseBody_(falseBody) {}

  ACCEPT_VISITOR(IfStmt, Stmt);

  REF_GETTER(Expr, Cond, *cond_);
  REF_GETTER(Stmt, TrueBody, *trueBody_);
  REF_GETTER(Stmt, FalseBody, *falseBody_);

 private:
  DISALLOW_COPY_AND_ASSIGN(IfStmt);

  uptr<Expr> cond_;
  uptr<Stmt> trueBody_;
  uptr<Stmt> falseBody_;
};

class ForStmt : public Stmt {
 public:
  ForStmt(Stmt* init, Expr* cond, Expr* update, Stmt* body)
      : init_(init), cond_(cond), update_(update), body_(body) {}

  ACCEPT_VISITOR(ForStmt, Stmt);

  REF_GETTER(Stmt, Init, *init_);
  VAL_GETTER(const Expr*, Cond, cond_.get());
  VAL_GETTER(const Expr*, Update, update_.get());
  REF_GETTER(Stmt, Body, *body_);

 private:
  DISALLOW_COPY_AND_ASSIGN(ForStmt);

  uptr<Stmt> init_;
  uptr<Expr> cond_;    // May be nullptr.
  uptr<Expr> update_;  // May be nullptr.
  uptr<Stmt> body_;
};

class WhileStmt : public Stmt {
 public:
  WhileStmt(Expr* cond, Stmt* body) : cond_(cond), body_(body) {}

  ACCEPT_VISITOR(WhileStmt, Stmt);

  REF_GETTER(Expr, Cond, *cond_);
  REF_GETTER(Stmt, Body, *body_);

 private:
  DISALLOW_COPY_AND_ASSIGN(WhileStmt);

  uptr<Expr> cond_;
  uptr<Stmt> body_;
};

class ModifierList {
 public:
  ModifierList()
      : mods_(int(lexer::NUM_MODIFIERS),
              lexer::Token(lexer::K_NULL, base::PosRange(0, 0, 0))) {}

  ModifierList(const ModifierList&) = default;

  void PrintTo(std::ostream* os) const {
    for (int i = 0; i < lexer::NUM_MODIFIERS; ++i) {
      if (!HasModifier((lexer::Modifier)i)) {
        continue;
      }
      *os << mods_[i].TypeInfo() << ' ';
    }
  }

  bool HasModifier(lexer::Modifier m) const {
    return mods_[m].TypeInfo().IsModifier();
  }

  bool AddModifier(lexer::Token t) {
    if (!t.TypeInfo().IsModifier()) {
      return false;
    }
    lexer::Modifier m = t.TypeInfo().GetModifier();
    if (HasModifier(m)) {
      return false;
    }
    mods_[m] = t;
    return true;
  }

  lexer::Token GetModifierToken(lexer::Modifier m) const {
    assert(HasModifier(m));
    return mods_[m];
  }

 private:
  vector<lexer::Token> mods_;
};

class Param final {
 public:
  Param(Type* type, lexer::Token ident) : type_(type), ident_(ident) {}

  ACCEPT_VISITOR(Param, Param);

  REF_GETTER(Type, GetType, *type_);
  VAL_GETTER(lexer::Token, Ident, ident_);

 private:
  DISALLOW_COPY_AND_ASSIGN(Param);

  uptr<Type> type_;
  lexer::Token ident_;
};

class ParamList final {
 public:
  ParamList(base::UniquePtrVector<Param>&& params)
      : params_(std::forward<base::UniquePtrVector<Param>>(params)) {}
  ~ParamList() = default;
  ParamList(ParamList&&) = default;

  ACCEPT_VISITOR(ParamList, ParamList);

  REF_GETTER(base::UniquePtrVector<Param>, Params, params_);

 private:
  DISALLOW_COPY_AND_ASSIGN(ParamList);

  base::UniquePtrVector<Param> params_;
};

class MemberDecl {
 public:
  MemberDecl(ModifierList&& mods, lexer::Token ident)
      : mods_(std::forward<ModifierList>(mods)), ident_(ident) {}
  virtual ~MemberDecl() = default;

  ACCEPT_VISITOR_ABSTRACT(MemberDecl);

  REF_GETTER(ModifierList, Mods, mods_);
  VAL_GETTER(lexer::Token, Ident, ident_);

 private:
  DISALLOW_COPY_AND_ASSIGN(MemberDecl);

  ModifierList mods_;
  lexer::Token ident_;
};

class ConstructorDecl : public MemberDecl {
 public:
  ConstructorDecl(ModifierList&& mods, lexer::Token ident, ParamList&& params,
                  Stmt* body)
      : MemberDecl(std::forward<ModifierList>(mods), ident),
        params_(std::forward<ParamList>(params)),
        body_(body) {}

  ACCEPT_VISITOR(ConstructorDecl, MemberDecl);

  REF_GETTER(ParamList, Params, params_);
  REF_GETTER(Stmt, Body, *body_);

 private:
  DISALLOW_COPY_AND_ASSIGN(ConstructorDecl);

  ModifierList mods_;
  ParamList params_;
  uptr<Stmt> body_;
};

class FieldDecl : public MemberDecl {
 public:
  FieldDecl(ModifierList&& mods, Type* type, lexer::Token ident, Expr* val)
      : MemberDecl(std::forward<ModifierList>(mods), ident),
        type_(type),
        val_(val) {}

  ACCEPT_VISITOR(FieldDecl, MemberDecl);

  REF_GETTER(Type, GetType, *type_);
  VAL_GETTER(const Expr*, Val, val_.get());

 private:
  DISALLOW_COPY_AND_ASSIGN(FieldDecl);

  uptr<Type> type_;
  uptr<Expr> val_;  // Might be nullptr.
};

class MethodDecl : public MemberDecl {
 public:
  MethodDecl(ModifierList&& mods, Type* type, lexer::Token ident,
             ParamList&& params, Stmt* body)
      : MemberDecl(std::forward<ModifierList>(mods), ident),
        type_(type),
        params_(std::forward<ParamList>(params)),
        body_(body) {}

  ACCEPT_VISITOR(MethodDecl, MemberDecl);

  REF_GETTER(Type, GetType, *type_);
  REF_GETTER(ParamList, Params, params_);
  REF_GETTER(Stmt, Body, *body_);

 private:
  DISALLOW_COPY_AND_ASSIGN(MethodDecl);

  uptr<Type> type_;
  ParamList params_;
  uptr<Stmt> body_;
};

enum class TypeKind {
  CLASS,
  INTERFACE,
};

class TypeDecl {
 public:
  virtual ~TypeDecl() = default;

  ACCEPT_VISITOR_ABSTRACT(TypeDecl);

  REF_GETTER(ModifierList, Mods, mods_);
  REF_GETTER(string, Name, name_);
  VAL_GETTER(lexer::Token, NameToken, nameToken_);
  REF_GETTER(vector<QualifiedName>, Interfaces, interfaces_);
  REF_GETTER(base::UniquePtrVector<MemberDecl>, Members, members_);
  VAL_GETTER(TypeId, GetTypeId, tid_);

 protected:
  TypeDecl(ModifierList&& mods, const string& name, lexer::Token nameToken,
           const vector<QualifiedName>& interfaces,
           base::UniquePtrVector<MemberDecl>&& members, TypeId tid)
      : mods_(std::forward<ModifierList>(mods)),
        name_(name),
        nameToken_(nameToken),
        interfaces_(interfaces),
        members_(std::forward<base::UniquePtrVector<MemberDecl>>(members)),
        tid_(tid) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TypeDecl);

  ModifierList mods_;
  string name_;
  lexer::Token nameToken_;
  vector<QualifiedName> interfaces_;
  base::UniquePtrVector<MemberDecl> members_;
  TypeId tid_;
};

class ClassDecl : public TypeDecl {
 public:
  ClassDecl(ModifierList&& mods, const string& name, lexer::Token nameToken,
            const vector<QualifiedName>& interfaces,
            base::UniquePtrVector<MemberDecl>&& members, ReferenceType* super, TypeId tid = TypeId::Unassigned())
      : TypeDecl(std::forward<ModifierList>(mods), name, nameToken,
                 interfaces,
                 std::forward<base::UniquePtrVector<MemberDecl>>(members), tid),
        super_(super) {}

  ACCEPT_VISITOR(ClassDecl, TypeDecl);

  VAL_GETTER(const ReferenceType*, Super, super_.get());

 private:
  DISALLOW_COPY_AND_ASSIGN(ClassDecl);

  uptr<ReferenceType> super_;  // Might be nullptr.
};

class InterfaceDecl : public TypeDecl {
 public:
  InterfaceDecl(ModifierList&& mods, const string& name, lexer::Token nameToken,
                const vector<QualifiedName>& interfaces,
                base::UniquePtrVector<MemberDecl>&& members, TypeId tid = TypeId::Unassigned())
      : TypeDecl(std::forward<ModifierList>(mods), name, nameToken,
                 interfaces,
                 std::forward<base::UniquePtrVector<MemberDecl>>(members), tid) {}

  ACCEPT_VISITOR(InterfaceDecl, TypeDecl);

 private:
  DISALLOW_COPY_AND_ASSIGN(InterfaceDecl);
};

class ImportDecl final {
 public:
  ImportDecl(const QualifiedName& name, bool isWildCard)
      : name_(name), isWildCard_(isWildCard) {}
  ImportDecl(const ImportDecl&) = default;

  REF_GETTER(QualifiedName, Name, name_);
  VAL_GETTER(bool, IsWildCard, isWildCard_);

 private:
  QualifiedName name_;
  bool isWildCard_;
};

class CompUnit final {
 public:
  CompUnit(QualifiedName* package, const vector<ImportDecl>& imports,
           base::UniquePtrVector<TypeDecl>&& types)
      : package_(package),
        imports_(imports),
        types_(std::forward<base::UniquePtrVector<TypeDecl>>(types)) {}

  ACCEPT_VISITOR(CompUnit, CompUnit);

  VAL_GETTER(const QualifiedName*, Package, package_.get());
  REF_GETTER(vector<ImportDecl>, Imports, imports_);
  REF_GETTER(base::UniquePtrVector<TypeDecl>, Types, types_);

 private:
  uptr<QualifiedName> package_;  // Might be nullptr.
  vector<ImportDecl> imports_;
  base::UniquePtrVector<TypeDecl> types_;
};

class Program final {
 public:
  Program(base::UniquePtrVector<CompUnit>&& units)
      : units_(std::forward<base::UniquePtrVector<CompUnit>>(units)) {}

  ACCEPT_VISITOR(Program, Program);

  REF_GETTER(base::UniquePtrVector<CompUnit>, CompUnits, units_);

 private:
  DISALLOW_COPY_AND_ASSIGN(Program);

  base::UniquePtrVector<CompUnit> units_;
};

#undef ACCEPT_VISITOR_ABSTRACT
#undef ACCEPT_VISITOR
#undef REF_GETTER
#undef VAL_GETTER

}  // namespace ast

#endif
