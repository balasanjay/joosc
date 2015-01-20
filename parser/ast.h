#ifndef PARSER_AST_H
#define PARSER_AST_H

#include "lexer/lexer.h"

namespace parser {

/*
class CompilationUnit {
  PackageDecl* package_;
  vector<ImportDecls*> imports_;
  TypeDecl* type_;
};

class PackageDecl {
  QualifiedName name_;
};

class QualifiedName {
  vector<string> parts_;
};

class TypeDecl {

};

class ClassDecl : public TypeDecl {
  vector<Modfier> modifiers_;
  Identifier name_;
};

class IfaceDecl : public TypeDecl {

};
*/

class Expr {
public:
  virtual void PrintTo(std::ostream* os) const = 0;
};

class BinExpr : public Expr {
public:
  BinExpr(Expr* lhs, lexer::Token op, Expr* rhs) : op_(op), lhs_(lhs), rhs_(rhs) {
    assert(lhs != nullptr);
    assert(lexer::TokenTypeIsBinOp(op.type));
    assert(rhs != nullptr);
  }

  void PrintTo(std::ostream* os) const override {
    *os << '(';
    lhs_->PrintTo(os);
    *os << ' ' << TokenTypeToString(op_.type) << ' ';
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
    assert(lexer::TokenTypeIsUnaryOp(op.type));
    assert(rhs != nullptr);
  }

  void PrintTo(std::ostream* os) const override {
    *os << '(' << TokenTypeToString(op_.type) << ' ';
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
    *os << lexer::TokenTypeToString(token_.type);
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
    *os << lexer::TokenTypeToString(token_.type);
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

      *os << lexer::TokenTypeToString(tok.type);
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


void Parse(const vector<lexer::Token>* tokens);

} // namespace parser

#endif
