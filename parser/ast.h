#ifndef PARSER_AST_H
#define PARSER_AST_H

#include "lexer/lexer.h"
#include <iostream>

namespace parser {

class QualifiedName final {
public:
  QualifiedName(const vector<lexer::Token>& tokens, const vector<string>& parts, const string& name) : tokens_(tokens), parts_(parts), name_(name) {}

  void PrintTo(std::ostream* os) const  {
    *os << name_;
  }

private:
  DISALLOW_COPY_AND_ASSIGN(QualifiedName);

  vector<lexer::Token> tokens_; // [IDENTIFIER, DOT, IDENTIFIER, DOT, IDENTIFIER]
  vector<string> parts_; // ["java", "lang", "String"]
  string name_; // "java.lang.String"
};

class Expr {
public:
  virtual ~Expr() = default;

  virtual void PrintTo(std::ostream* os) const = 0;

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

    void PrintTo(std::ostream* os) const {
      for (int i = 0; i < args_.Size(); ++i) {
        if (i > 0) {
          *os << ", ";
        }
        args_.At(i)->PrintTo(os);
      }
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(ArgumentList);

    base::UniquePtrVector<Expr> args_;
};

class NameExpr : public Expr {
  public:
    NameExpr(QualifiedName* name) : name_(name) {}

    void PrintTo(std::ostream* os) const override {
      name_->PrintTo(os);
    }

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

  void PrintTo(std::ostream* os) const override {
    *os << '(';
    lhs_->PrintTo(os);
    *os << ' ' << op_.TypeInfo() << ' ';
    rhs_->PrintTo(os);
    *os << ')';
  }

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

  void PrintTo(std::ostream* os) const override {
    *os << '(' << op_.TypeInfo() << ' ';
    rhs_->PrintTo(os);
    *os << ')';
  }

private:
  lexer::Token op_;
  unique_ptr<Expr> rhs_;
};

class LitExpr : public Expr {
public :
  LitExpr(lexer::Token token) : token_(token) {}

  void PrintTo(std::ostream* os) const override {
    *os << token_.TypeInfo();
  }

private:
  lexer::Token token_;
};

class ThisExpr : public Expr {
public :
  void PrintTo(std::ostream* os) const override {
    *os << "this";
  }
private:
};

class ArrayIndexExpr : public Expr {
  public:
    ArrayIndexExpr(Expr* base, Expr* index) : base_(base), index_(index) {}

    void PrintTo(std::ostream* os) const override {
      base_->PrintTo(os);
      *os << '[';
      index_->PrintTo(os);
      *os << ']';
    }

  private:
    unique_ptr<Expr> base_;
    unique_ptr<Expr> index_;
};

class FieldDerefExpr : public Expr {
  public:
    FieldDerefExpr(Expr* base, const string& fieldname, lexer::Token token) : base_(base), fieldname_(fieldname), token_(token) {}

    void PrintTo(std::ostream* os) const override {
      base_->PrintTo(os);
      *os << '.' << fieldname_;
    }

  private:
    unique_ptr<Expr> base_;
    string fieldname_;
    lexer::Token token_;
};

class CallExpr : public Expr {
  public:
    CallExpr(Expr* base, ArgumentList* args) : base_(base), args_(args) {}

  void PrintTo(std::ostream* os) const override {
    base_->PrintTo(os);
    *os << '(';
    args_->PrintTo(os);
    *os << ')';
  }

  private:
    unique_ptr<Expr> base_;
    unique_ptr<ArgumentList> args_;
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

private:
  DISALLOW_COPY_AND_ASSIGN(ArrayType);

  unique_ptr<Type> elemtype_;
};

class CastExpr : public Expr {
  public:
    CastExpr(Type* type, Expr* expr) : type_(type), expr_(expr) {}
    void PrintTo(std::ostream* os) const override {
      *os << "cast<" ;
      type_->PrintTo(os);
      *os << ">(";
      expr_->PrintTo(os);
      *os << ')';
    }
  private:
    DISALLOW_COPY_AND_ASSIGN(CastExpr);

    unique_ptr<Type> type_;
    unique_ptr<Expr> expr_;
};

class NewClassExpr : public Expr {
public:
  NewClassExpr(Type* type, ArgumentList* args) : type_(type), args_(args) {}

  void PrintTo(std::ostream* os) const override {
    *os << "new<";
    type_->PrintTo(os);
    *os << ">(";
    args_->PrintTo(os);
    *os << ")";
  }

private:
  DISALLOW_COPY_AND_ASSIGN(NewClassExpr);

  unique_ptr<Type> type_;
  unique_ptr<ArgumentList> args_;
};

class NewArrayExpr : public Expr {
public:
  NewArrayExpr(Type* type, Expr* expr) : type_(type), expr_(expr) {}

  void PrintTo(std::ostream* os) const override {
    *os << "new<array<";
    type_->PrintTo(os);

    *os << ">>(";
    if (expr_ != nullptr) {
      expr_->PrintTo(os);
    }
    *os << ")";
  }

private:
  DISALLOW_COPY_AND_ASSIGN(NewArrayExpr);

  unique_ptr<Type> type_;
  unique_ptr<Expr> expr_;
};

class Stmt {
public:
  virtual ~Stmt() = default;

  virtual void PrintTo(std::ostream* os) const = 0;

protected:
  Stmt() = default;

private:
  DISALLOW_COPY_AND_ASSIGN(Stmt);
};

class EmptyStmt : public Stmt {
public:
  EmptyStmt() = default;

  void PrintTo(std::ostream* os) const override {
    *os << ";";
  }
};

class LocalDeclStmt : public Stmt {
public:
  LocalDeclStmt(Type* type, lexer::Token ident, Expr* val): type_(type), ident_(ident), val_(val) {}

  void PrintTo(std::ostream* os) const override {
    type_->PrintTo(os);
    *os << " " << ident_.TypeInfo() << " = ";
    val_->PrintTo(os);
  }

private:
  unique_ptr<Type> type_;
  lexer::Token ident_;
  unique_ptr<Expr> val_;
};

class ReturnStmt : public Stmt {
public:
  ReturnStmt(Expr* val): val_(val) {}

  void PrintTo(std::ostream* os) const override {
    *os << "return";
    if (val_ != nullptr) {
      *os << " ";
      val_->PrintTo(os);
    }
    *os << ";";
  }

private:
  unique_ptr<Expr> val_;
};

class ExprStmt : public Stmt {
public:
  ExprStmt(Expr* expr): expr_(expr) {}

  void PrintTo(std::ostream* os) const override {
    expr_->PrintTo(os);
    *os << ";";
  }

private:
  unique_ptr<Expr> expr_;
};

class BlockStmt : public Stmt {
public:
  BlockStmt(base::UniquePtrVector<Stmt>&& stmts): stmts_(std::forward<base::UniquePtrVector<Stmt>>(stmts)) {}

  void PrintTo(std::ostream* os) const override {
    *os << "{";
    for (int i = 0; i < stmts_.Size(); i++) {
      stmts_.At(i)->PrintTo(os);
      // *os << "\n";
    }
    *os << "}";
  }

private:
  DISALLOW_COPY_AND_ASSIGN(BlockStmt);
  base::UniquePtrVector<Stmt> stmts_;
};

class IfStmt : public Stmt {
public:
  IfStmt(Expr* cond, Stmt* trueBody, Stmt* falseBody): cond_(cond), trueBody_(trueBody), falseBody_(falseBody) {}

  void PrintTo(std::ostream* os) const override {
    *os << "if(";
    cond_->PrintTo(os);
    *os << "){";
    trueBody_->PrintTo(os);
    *os << "}else{";
    falseBody_->PrintTo(os);
    *os << "}";
  }

private:
  DISALLOW_COPY_AND_ASSIGN(IfStmt);
  unique_ptr<Expr> cond_;
  unique_ptr<Stmt> trueBody_;
  unique_ptr<Stmt> falseBody_;
};

class ForStmt : public Stmt {
public:
  ForStmt(Stmt* init, Expr* cond, Stmt* update, Stmt* body): init_(init), cond_(cond), update_(update), body_(body) {}

  void PrintTo(std::ostream* os) const override {
    *os << "for(";
    init_->PrintTo(os);
    // Stmt prints semicolon.
    if (cond_ != nullptr) {
      cond_->PrintTo(os);
    }
    *os << ";";
    update_->PrintTo(os);
    // TODO: Prints ending semi :(
    *os << "){";
    body_->PrintTo(os);
    *os << "}";
  }

private:
  DISALLOW_COPY_AND_ASSIGN(ForStmt);
  unique_ptr<Stmt> init_; // May be EmptyStmt.
  unique_ptr<Expr> cond_; // May be null.
  unique_ptr<Stmt> update_; // May be EmptyStmt.
  unique_ptr<Stmt> body_; // May be EmptyStmt.
};


void Parse(const base::FileSet* fs, const base::File* file, const vector<lexer::Token>* tokens);

} // namespace parser

#endif
