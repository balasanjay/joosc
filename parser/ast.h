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

class Type {
public:
  virtual ~Type() = default;

  virtual void PrintTo(std::ostream* os) const = 0;
};

class PrimitiveType : public Type {
public:
  PrimitiveType(lexer::Token token) : token_(token) {}

  void PrintTo(std::ostream* os) const override {
    *os << token_.TypeInfo();
  }

private:
  lexer::Token token_;
};

class ReferenceType : public Type {
public:
  ReferenceType(QualifiedName* name) : name_(name) {}

  void PrintTo(std::ostream* os) const override {
    name_->PrintTo(os);
  }

private:
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
    unique_ptr<Type> type_;
    unique_ptr<Expr> expr_;
};


void Parse(const base::File* file, const vector<lexer::Token>* tokens);

} // namespace parser

#endif
