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

class ConstExpr : public Expr {
public :
  // ConstExpr(lexer::Token token) : token_(token) {}
  void PrintTo(std::ostream* os) const override {
    *os << "IDENTIFIER";
  }
private:
  // lexer::Token token_;
};

void Parse(const vector<lexer::Token>* tokens);

} // namespace parser

#endif
