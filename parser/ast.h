#ifndef PARSER_AST_H
#define PARSER_AST_H

#include "lexer/lexer.h"
#include "parser/visitor.h"
#include <iostream>

namespace parser {

#define ACCEPT_VISITOR(type) void Accept(Visitor* visitor) const override { visitor->Visit##type(this); }

class QualifiedName final {
public:
  QualifiedName(const vector<lexer::Token>& tokens, const vector<string>& parts, const string& name) : tokens_(tokens), parts_(parts), name_(name) {}

  void PrintTo(std::ostream* os) const  {
    *os << name_;
  }

  const string& Name() const { return name_; }

private:
  DISALLOW_COPY_AND_ASSIGN(QualifiedName);

  vector<lexer::Token> tokens_; // [IDENTIFIER, DOT, IDENTIFIER, DOT, IDENTIFIER]
  vector<string> parts_; // ["java", "lang", "String"]
  string name_; // "java.lang.String"
};

class Type {
public:
  virtual ~Type() = default;

  virtual void PrintTo(std::ostream* os) const = 0;

protected:
  Type() = default;

private:
  DISALLOW_COPY_AND_ASSIGN(Type);
};

class PrimitiveType : public Type {
public:
  PrimitiveType(lexer::Token token) : token_(token) {}

  void PrintTo(std::ostream* os) const override {
    *os << token_.TypeInfo();
  }

  lexer::Token GetToken() const { return token_; }

private:
  DISALLOW_COPY_AND_ASSIGN(PrimitiveType);

  lexer::Token token_;
};

class ReferenceType : public Type {
public:
  ReferenceType(QualifiedName* name) : name_(name) {}

  void PrintTo(std::ostream* os) const override {
    name_->PrintTo(os);
  }

  const QualifiedName* Name() const { return name_.get(); }

private:
  DISALLOW_COPY_AND_ASSIGN(ReferenceType);

  unique_ptr<QualifiedName> name_;
};

class ArrayType : public Type {
public:
  ArrayType(Type* elemtype) : elemtype_(elemtype) {}

  void PrintTo(std::ostream* os) const override {
    *os << "array<";
    elemtype_->PrintTo(os);
    *os << '>';
  }

  const Type* ElemType() const { return elemtype_.get(); }

private:
  DISALLOW_COPY_AND_ASSIGN(ArrayType);

  unique_ptr<Type> elemtype_;
};


class Expr {
public:
  virtual ~Expr() = default;

  virtual void Accept(Visitor* visitor) const = 0;

protected:
  Expr() = default;

private:
  DISALLOW_COPY_AND_ASSIGN(Expr);
};

class ArgumentList final {
  public:
    ArgumentList(base::UniquePtrVector<Expr>&& args) : args_(std::forward<base::UniquePtrVector<Expr>>(args)) {}
    ~ArgumentList() = default;
    ArgumentList(ArgumentList&&) = default;

    void Accept(Visitor* visitor) const { visitor->VisitArgumentList(this); }

    const base::UniquePtrVector<Expr>& Args() const { return args_; }

  private:
    DISALLOW_COPY_AND_ASSIGN(ArgumentList);

    base::UniquePtrVector<Expr> args_;
};



class NameExpr : public Expr {
  public:
    NameExpr(QualifiedName* name) : name_(name) {}

    ACCEPT_VISITOR(NameExpr);

    const QualifiedName* Name() const { return name_.get(); }

  private:
    unique_ptr<QualifiedName> name_;
};

class BinExpr : public Expr {
public:
  BinExpr(Expr* lhs, lexer::Token op, Expr* rhs) : op_(op), lhs_(lhs), rhs_(rhs) {
    assert(lhs != nullptr);
    assert(op.TypeInfo().IsBinOp());
    assert(rhs != nullptr);
  }

  ACCEPT_VISITOR(BinExpr);

  const Expr* Lhs() const { return lhs_.get(); }
  const Expr* Rhs() const { return rhs_.get(); }
  lexer::Token Op() const { return op_; }

private:
  lexer::Token op_;

  unique_ptr<Expr> lhs_;
  unique_ptr<Expr> rhs_;
};

class UnaryExpr : public Expr {
public:
  UnaryExpr(lexer::Token op, Expr* rhs) : op_(op), rhs_(rhs) {
    assert(op.TypeInfo().IsUnaryOp());
    assert(rhs != nullptr);
  }

  ACCEPT_VISITOR(UnaryExpr);

  lexer::Token Op() const { return op_; }
  const Expr* Rhs() const { return rhs_.get(); }

private:
  lexer::Token op_;
  unique_ptr<Expr> rhs_;
};

class LitExpr : public Expr {
public :
  LitExpr(lexer::Token token) : token_(token) {}

  ACCEPT_VISITOR(LitExpr);

  lexer::Token GetToken() const { return token_; }

private:
  lexer::Token token_;
};

class ThisExpr : public Expr {
public :
  ACCEPT_VISITOR(ThisExpr);
};

class ArrayIndexExpr : public Expr {
  public:
    ArrayIndexExpr(Expr* base, Expr* index) : base_(base), index_(index) {}

    ACCEPT_VISITOR(ArrayIndexExpr);

    const Expr* Base() const { return base_.get(); }
    const Expr* Index() const { return index_.get(); }

  private:
    unique_ptr<Expr> base_;
    unique_ptr<Expr> index_;
};

class FieldDerefExpr : public Expr {
  public:
    FieldDerefExpr(Expr* base, const string& fieldname, lexer::Token token) : base_(base), fieldname_(fieldname), token_(token) {}

    ACCEPT_VISITOR(FieldDerefExpr);

    const Expr* Base() const { return base_.get(); }
    const string& FieldName() const { return fieldname_; }

  private:
    unique_ptr<Expr> base_;
    string fieldname_;
    lexer::Token token_;
};

class CallExpr : public Expr {
  public:
    CallExpr(Expr* base, lexer::Token lparen, ArgumentList* args) : base_(base), lparen_(lparen), args_(args) {}

    ACCEPT_VISITOR(CallExpr);

    const Expr* Base() const { return base_.get(); }
    lexer::Token Lparen() const { return lparen_; }
    const ArgumentList* Args() const { return args_.get(); }

  private:
    unique_ptr<Expr> base_;
    lexer::Token lparen_;
    unique_ptr<ArgumentList> args_;
};

class CastExpr : public Expr {
  public:
    CastExpr(Type* type, Expr* expr) : type_(type), expr_(expr) {}

    ACCEPT_VISITOR(CastExpr);

    const Type* GetType() const { return type_.get(); }
    const Expr* GetExpr() const { return expr_.get(); }

  private:
    DISALLOW_COPY_AND_ASSIGN(CastExpr);

    unique_ptr<Type> type_;
    unique_ptr<Expr> expr_;
};

class NewClassExpr : public Expr {
public:
  NewClassExpr(lexer::Token newTok, Type* type, ArgumentList* args) : newTok_(newTok), type_(type), args_(args) {}

  ACCEPT_VISITOR(NewClassExpr);

  lexer::Token NewToken() const { return newTok_; }
  const Type* GetType() const { return type_.get(); }
  const ArgumentList* Args() const { return args_.get(); }

private:
  DISALLOW_COPY_AND_ASSIGN(NewClassExpr);

  lexer::Token newTok_;
  unique_ptr<Type> type_;
  unique_ptr<ArgumentList> args_;
};

class NewArrayExpr : public Expr {
public:
  NewArrayExpr(Type* type, Expr* expr) : type_(type), expr_(expr) {}

  ACCEPT_VISITOR(NewArrayExpr);

  const Type* GetType() const { return type_.get(); }
  const Expr* GetExpr() const { return expr_.get(); }

private:
  DISALLOW_COPY_AND_ASSIGN(NewArrayExpr);

  unique_ptr<Type> type_;
  unique_ptr<Expr> expr_;
};

class Stmt {
public:
  virtual ~Stmt() = default;

  virtual void Accept(Visitor* visitor) const = 0;

protected:
  Stmt() = default;

private:
  DISALLOW_COPY_AND_ASSIGN(Stmt);
};

class EmptyStmt : public Stmt {
public:
  EmptyStmt() = default;

  ACCEPT_VISITOR(EmptyStmt);
};

class LocalDeclStmt : public Stmt {
public:
  LocalDeclStmt(Type* type, lexer::Token ident, Expr* val): type_(type), ident_(ident), val_(val) {}

  ACCEPT_VISITOR(LocalDeclStmt);

  const Type* GetType() const { return type_.get(); }
  lexer::Token Ident() const { return ident_; }
  const Expr* GetExpr() const { return val_.get(); }



  // TODO: get the identifier as a string.

private:
  unique_ptr<Type> type_;
  lexer::Token ident_;
  unique_ptr<Expr> val_;
};

class ReturnStmt : public Stmt {
public:
  ReturnStmt(Expr* expr): expr_(expr) {}

  ACCEPT_VISITOR(ReturnStmt);

  const Expr* GetExpr() const { return expr_.get(); }

private:
  unique_ptr<Expr> expr_;
};

class ExprStmt : public Stmt {
public:
  ExprStmt(Expr* expr): expr_(expr) {}

  ACCEPT_VISITOR(ExprStmt);

  const Expr* GetExpr() const { return expr_.get(); }

private:
  unique_ptr<Expr> expr_;
};

class BlockStmt : public Stmt {
public:
  BlockStmt(base::UniquePtrVector<Stmt>&& stmts): stmts_(std::forward<base::UniquePtrVector<Stmt>>(stmts)) {}

  ACCEPT_VISITOR(BlockStmt);

  const base::UniquePtrVector<Stmt>& Stmts() const { return stmts_; }

private:
  DISALLOW_COPY_AND_ASSIGN(BlockStmt);
  base::UniquePtrVector<Stmt> stmts_;
};

class IfStmt : public Stmt {
public:
  IfStmt(Expr* cond, Stmt* trueBody, Stmt* falseBody): cond_(cond), trueBody_(trueBody), falseBody_(falseBody) {}

  ACCEPT_VISITOR(IfStmt);

  const Expr* Cond() const { return cond_.get(); }
  const Stmt* TrueBody() const { return trueBody_.get(); }
  const Stmt* FalseBody() const { return falseBody_.get(); }

private:
  DISALLOW_COPY_AND_ASSIGN(IfStmt);

  unique_ptr<Expr> cond_;
  unique_ptr<Stmt> trueBody_;
  unique_ptr<Stmt> falseBody_;
};

class ForStmt : public Stmt {
public:
  ForStmt(Stmt* init, Expr* cond, Expr* update, Stmt* body): init_(init), cond_(cond), update_(update), body_(body) {}

  ACCEPT_VISITOR(ForStmt);

  const Stmt* Init() const { return init_.get(); }
  const Expr* Cond() const { return cond_.get(); }
  const Expr* Update() const { return update_.get(); }
  const Stmt* Body() const { return body_.get(); }

private:
  DISALLOW_COPY_AND_ASSIGN(ForStmt);

  unique_ptr<Stmt> init_; // May be EmptyStmt.
  unique_ptr<Expr> cond_; // May be nullptr.
  unique_ptr<Expr> update_; // May be nullptr.
  unique_ptr<Stmt> body_; // May be EmptyStmt.
};

class ModifierList {
public:
  ModifierList(): mods_(int(lexer::NUM_MODIFIERS), lexer::Token(lexer::K_NULL, base::PosRange(0, 0, 0))) {}

  void PrintTo(std::ostream* os) const  {
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
  Param(Type* type, lexer::Token ident): type_(type), ident_(ident) {}

  void Accept(Visitor* visitor) const { visitor->VisitParam(this); }

  const Type* GetType() const { return type_.get(); }
  lexer::Token Ident() const { return ident_; }

private:
  DISALLOW_COPY_AND_ASSIGN(Param);

  unique_ptr<Type> type_;
  lexer::Token ident_;
};

class ParamList final {
public:
  ParamList(base::UniquePtrVector<Param>&& params): params_(std::forward<base::UniquePtrVector<Param>>(params)) {}
  ~ParamList() = default;
  ParamList(ParamList&&) = default;

  void Accept(Visitor* visitor) const { visitor->VisitParamList(this); }

  const base::UniquePtrVector<Param>& Params() const { return params_; }

private:
  DISALLOW_COPY_AND_ASSIGN(ParamList);

  base::UniquePtrVector<Param> params_;
};

class MemberDecl {
public:
  MemberDecl(ModifierList&& mods, Type* type, lexer::Token ident): mods_(std::forward<ModifierList>(mods)), type_(type), ident_(ident) {}

  virtual ~MemberDecl() = default;

  virtual void Accept(Visitor* visitor) const = 0;

  const ModifierList& Mods() const { return mods_; }
  const Type* GetType() const { return type_.get(); }
  lexer::Token Ident() const { return ident_; }

private:
  ModifierList mods_;
  unique_ptr<Type> type_;
  lexer::Token ident_;
};

class FieldDecl : public MemberDecl {
public:
  FieldDecl(ModifierList&& mods, Type* type, lexer::Token ident, Expr* val): MemberDecl(std::forward<ModifierList>(mods), type, ident), val_(val) {}

  ACCEPT_VISITOR(FieldDecl);

  const Expr* Val() const { return val_.get(); }

private:
  unique_ptr<Expr> val_; // Might be nullptr.
};

class MethodDecl : public MemberDecl {
public:
  MethodDecl(ModifierList&& mods, Type* type, lexer::Token ident, ParamList&& params, Stmt* body): MemberDecl(std::forward<ModifierList>(mods), type, ident), params_(std::forward<ParamList>(params)), body_(body) {}

  ACCEPT_VISITOR(MethodDecl);

  const ParamList& Params() const { return params_; }
  const Stmt* Body() const { return body_.get(); }

private:
  ParamList params_;
  unique_ptr<Stmt> body_;
};


#undef ACCEPT_VISITOR

void Parse(const base::FileSet* fs, const base::File* file, const vector<lexer::Token>* tokens);

} // namespace parser

#endif
