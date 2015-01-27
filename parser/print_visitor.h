#ifndef PARSER_PRINT_VISITOR_H
#define PARSER_PRINT_VISITOR_H

#include "parser/ast.h"
#include "parser/visitor.h"

namespace parser {

class PrintVisitor final : public Visitor {
public:
  static PrintVisitor Pretty(std::ostream* os) {
    return PrintVisitor(os, 0, "\n", "  ", " ");
  }

  static PrintVisitor Compact(std::ostream* os) {
    return PrintVisitor(os, 0, "", "", "");
  }

  VISIT(ArrayIndexExpr, expr) {
    expr->Base()->Accept(this);
    *os_ << '[';
    expr->Index()->Accept(this);
    *os_ << ']';
  }

  VISIT(BinExpr, expr) {
    *os_ << '(';
    expr->Lhs()->Accept(this);
    *os_ << ' ' << expr->Op().TypeInfo() << ' ';
    expr->Rhs()->Accept(this);
    *os_ << ')';
  }

  VISIT(CallExpr, expr) {
    expr->Base()->Accept(this);
    *os_ << '(';
    expr->Args()->Accept(this);
    *os_ << ')';
  }

  VISIT(CastExpr, expr) {
    *os_ << "cast<" ;
    expr->GetType()->PrintTo(os_);
    *os_ << ">(";
    expr->GetExpr()->Accept(this);
    *os_ << ')';
  }

  VISIT(FieldDerefExpr, expr) {
    expr->Base()->Accept(this);
    *os_ << '.' << expr->FieldName();
  }

  VISIT(LitExpr, expr) {
    *os_ << expr->GetToken().TypeInfo();
  }

  VISIT(NameExpr, expr) {
    *os_ << expr->Name()->Name();
  }

  VISIT(NewArrayExpr, expr) {
    *os_ << "new<array<";
    expr->GetType()->PrintTo(os_);

    *os_ << ">>(";
    if (expr->GetExpr() != nullptr) {
      expr->GetExpr()->Accept(this);
    }
    *os_ << ")";
  }

  VISIT(NewClassExpr, expr) {
    *os_ << "new<";
    expr->GetType()->PrintTo(os_);
    *os_ << ">(";
    expr->Args()->Accept(this);
    *os_ << ")";
  }

  VISIT(ThisExpr,) {
    *os_ << "this";
  }

  VISIT(UnaryExpr, expr) {
    *os_ << '(' << expr->Op().TypeInfo() << ' ';
    expr->Rhs()->Accept(this);
    *os_ << ')';
  }

  VISIT(BlockStmt, stmt) {
    *os_ << "{" << newline_;
    PrintVisitor nested = Indent();
    const auto& stmts = stmt->Stmts();
    for (int i = 0; i < stmts.Size(); ++i) {
      PutIndent(depth_ + 1);
      stmts.At(i)->Accept(&nested);
      *os_ << newline_;
    }

    PutIndent(depth_);
    *os_ << '}';
  }

  VISIT(EmptyStmt,) {
    *os_ << ';';
  }

  VISIT(ExprStmt, stmt) {
    stmt->GetExpr()->Accept(this);
    *os_ << ';';
  }

  VISIT(LocalDeclStmt, stmt) {
    stmt->GetType()->PrintTo(os_);
    *os_ << ' ' << stmt->Ident().TypeInfo() << space_ << '=' << space_;
    stmt->GetExpr()->Accept(this);
    *os_ << ';';
  }

  VISIT(ReturnStmt, stmt) {
   *os_ << "return";
    if (stmt->GetExpr() != nullptr) {
      *os_ << ' ';
      stmt->GetExpr()->Accept(this);
    }
    *os_ << ';';
  }

  VISIT(IfStmt, stmt) {
    *os_ << "if" << space_ << '(';
    stmt->Cond()->Accept(this);
    *os_ << ')' << space_ << '{';
    stmt->TrueBody()->Accept(this);
    *os_ << '}' << space_ << "else" << space_ << '{';
    stmt->FalseBody()->Accept(this);
    *os_ << '}';
  }

  VISIT(ForStmt, stmt) {
    *os_ << "for" << space_ << '(';
    stmt->Init()->Accept(this);
    if (stmt->Cond() != nullptr) {
      *os_ << space_;
      stmt->Cond()->Accept(this);
    }
    *os_ << ';';
    if (stmt->Update() != nullptr) {
      *os_ << space_;
      stmt->Update()->Accept(this);
    }
    *os_ << ')' << space_ << '{';
    stmt->Body()->Accept(this);
    *os_ << '}';
  }

  VISIT(ArgumentList, args) {
    for (int i = 0; i < args->Args().Size(); ++i) {
      if (i > 0) {
        *os_ << ',' << space_;
      }
      args->Args().At(i)->Accept(this);
    }
  }

private:
  PrintVisitor(std::ostream* os, int depth, const string& newline, const string& tab, const string& space) : Visitor(), os_(os), depth_(depth), newline_(newline), tab_(tab), space_(space) {}

  PrintVisitor Indent() const {
    return PrintVisitor(os_, depth_ + 1, newline_, tab_, space_);
  }

  void PutIndent(int depth) {
    for (int i = 0; i < depth; ++i) {
      *os_ << tab_;
    }
  }

  std::ostream* os_;
  int depth_;
  string newline_;
  string tab_;
  string space_;
};



} // namespace parser

#endif
