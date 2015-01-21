#ifndef PARSER_AST_H
#define PARSER_AST_H

#include "lexer/lexer.h"

namespace parser {

class QualifiedName {

private:
  DISALLOW_COPY_AND_ASSIGN(QualifiedName);

  vector<lexer::Token> tokens_; // Alternating IDENTIFIER and DOT.
  vector<string> names_;
  string fullname_;
};

class Expr {
public:
  virtual void PrintTo(std::ostream* os) const = 0;

protected:
  Expr() {}

private:
  DISALLOW_COPY_AND_ASSIGN(Expr);
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
    *os << "THIS";
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

class Type {
public:
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
  ReferenceType(vector<lexer::Token>* toks) : toks_(toks) {}

  void PrintTo(std::ostream* os) const override {
    *os << '(';
    bool first = true;
    for (const auto& tok : *toks_) {
      if (!first) {
        *os << ' ';
      }
      first = false;

      *os << tok.TypeInfo();
    }
    *os << ')';
  }
private:
  unique_ptr<vector<lexer::Token>> toks_;
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
