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
  void PrintTo(std::ostream* os) const override {
    *os << '(';
    lhs_->PrintTo(os);
    *os << ' ' << Op() << ' ';
    rhs_->PrintTo(os);
    *os << ')';
  }

protected:
  BinExpr(Expr* lhs, Expr* rhs) : lhs_(lhs), rhs_(rhs) {}

  virtual string Op() const = 0;

private:
  unique_ptr<Expr> lhs_;
  unique_ptr<Expr> rhs_;
};

class AddExpr : public BinExpr {
public:
  AddExpr(Expr* lhs, Expr* rhs) : BinExpr(lhs, rhs) {}

protected:
  string Op() const { return "+"; }
};

class MulExpr : public BinExpr {
public:
  MulExpr(Expr* lhs, Expr* rhs) : BinExpr(lhs, rhs) {}

protected:
  string Op() const { return "*"; }
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
