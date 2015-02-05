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

  VISIT_DECL(ArrayIndexExpr, expr) {
    expr->Base()->Accept(this);
    *os_ << '[';
    expr->Index()->Accept(this);
    *os_ << ']';
  }

  VISIT_DECL(BinExpr, expr) {
    *os_ << '(';
    expr->Lhs()->Accept(this);
    *os_ << ' ' << expr->Op().TypeInfo() << ' ';
    expr->Rhs()->Accept(this);
    *os_ << ')';
  }

  VISIT_DECL(CallExpr, expr) {
    expr->Base()->Accept(this);
    *os_ << '(';
    expr->Args()->Accept(this);
    *os_ << ')';
  }

  VISIT_DECL(CastExpr, expr) {
    *os_ << "cast<";
    expr->GetType()->PrintTo(os_);
    *os_ << ">(";
    expr->GetExpr()->Accept(this);
    *os_ << ')';
  }

  VISIT_DECL(InstanceOfExpr, expr) {
    *os_ << '(';
    expr->Lhs()->Accept(this);
    *os_ << " instanceof ";
    expr->GetType()->PrintTo(os_);
    *os_ << ')';
  }

  VISIT_DECL(FieldDerefExpr, expr) {
    expr->Base()->Accept(this);
    *os_ << '.' << expr->FieldName();
  }

  VISIT_DECL(BoolLitExpr, expr) { *os_ << expr->GetToken().TypeInfo(); }

  VISIT_DECL(StringLitExpr, expr) { *os_ << expr->GetToken().TypeInfo(); }

  VISIT_DECL(CharLitExpr, expr) { *os_ << expr->GetToken().TypeInfo(); }

  VISIT_DECL(NullLitExpr, expr) { *os_ << expr->GetToken().TypeInfo(); }

  VISIT_DECL(IntLitExpr, expr) { *os_ << expr->GetToken().TypeInfo(); }

  VISIT_DECL(NameExpr, expr) { *os_ << expr->Name().Name(); }

  VISIT_DECL(NewArrayExpr, expr) {
    *os_ << "new<array<";
    expr->GetType()->PrintTo(os_);

    *os_ << ">>(";
    if (expr->GetExpr() != nullptr) {
      expr->GetExpr()->Accept(this);
    }
    *os_ << ")";
  }

  VISIT_DECL(NewClassExpr, expr) {
    *os_ << "new<";
    expr->GetType()->PrintTo(os_);
    *os_ << ">(";
    expr->Args()->Accept(this);
    *os_ << ")";
  }

  VISIT_DECL(ParenExpr, expr) {
    *os_ << "(";
    expr->Nested()->Accept(this);
    *os_ << ")";
  }

  VISIT_DECL(ThisExpr, ) { *os_ << "this"; }

  VISIT_DECL(UnaryExpr, expr) {
    *os_ << '(' << expr->Op().TypeInfo() << ' ';
    expr->Rhs()->Accept(this);
    *os_ << ')';
  }

  VISIT_DECL(BlockStmt, stmt) {
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

  VISIT_DECL(EmptyStmt, ) { *os_ << ';'; }

  VISIT_DECL(ExprStmt, stmt) {
    stmt->GetExpr()->Accept(this);
    *os_ << ';';
  }

  VISIT_DECL(LocalDeclStmt, stmt) {
    stmt->GetType()->PrintTo(os_);
    *os_ << ' ' << stmt->Ident().TypeInfo() << space_ << '=' << space_;
    stmt->GetExpr()->Accept(this);
    *os_ << ';';
  }

  VISIT_DECL(ReturnStmt, stmt) {
    *os_ << "return";
    if (stmt->GetExpr() != nullptr) {
      *os_ << ' ';
      stmt->GetExpr()->Accept(this);
    }
    *os_ << ';';
  }

  VISIT_DECL(IfStmt, stmt) {
    *os_ << "if" << space_ << '(';
    stmt->Cond()->Accept(this);
    *os_ << ')' << space_ << '{';
    stmt->TrueBody()->Accept(this);
    *os_ << '}' << space_ << "else" << space_ << '{';
    stmt->FalseBody()->Accept(this);
    *os_ << '}';
  }

  VISIT_DECL(ForStmt, stmt) {
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

  VISIT_DECL(WhileStmt, stmt) {
    *os_ << "while" << space_ << '(';
    stmt->Cond()->Accept(this);
    *os_ << ')' << space_ << '{';
    stmt->Body()->Accept(this);
    *os_ << '}';
  }

  VISIT_DECL(ArgumentList, args) {
    for (int i = 0; i < args->Args().Size(); ++i) {
      if (i > 0) {
        *os_ << ',' << space_;
      }
      args->Args().At(i)->Accept(this);
    }
  }

  VISIT_DECL(ParamList, params) {
    for (int i = 0; i < params->Params().Size(); ++i) {
      if (i > 0) {
        *os_ << ',' << space_;
      }
      params->Params().At(i)->Accept(this);
    }
  }

  VISIT_DECL(Param, param) {
    param->GetType()->PrintTo(os_);
    *os_ << ' ' << param->Ident().TypeInfo();
  }

  VISIT_DECL(FieldDecl, field) {
    field->Mods().PrintTo(os_);
    field->GetType()->PrintTo(os_);
    *os_ << ' ';
    *os_ << field->Ident().TypeInfo();
    if (field->Val() != nullptr) {
      *os_ << space_ << '=' << space_;
      field->Val()->Accept(this);
    }
    *os_ << ';';
  }

  VISIT_DECL(ConstructorDecl, meth) {
    meth->Mods().PrintTo(os_);
    *os_ << meth->Ident().TypeInfo();
    *os_ << '(';
    meth->Params().Accept(this);
    *os_ << ')';
    meth->Body()->Accept(this);
  }

  VISIT_DECL(MethodDecl, meth) {
    meth->Mods().PrintTo(os_);
    meth->GetType()->PrintTo(os_);
    *os_ << ' ';
    *os_ << meth->Ident().TypeInfo();
    *os_ << '(';
    meth->Params().Accept(this);
    *os_ << ')';
    meth->Body()->Accept(this);
  }

  VISIT_DECL(ClassDecl, type) {
    type->Mods().PrintTo(os_);
    *os_ << "class ";
    *os_ << type->NameToken().TypeInfo();
    if (type->Super() != nullptr) {
      *os_ << " extends ";
      type->Super()->PrintTo(os_);
    }
    for (int i = 0; i < type->Interfaces().Size(); ++i) {
      if (i == 0) {
        *os_ << " implements ";
      } else {
        *os_ << ',' << space_;
      }
      type->Interfaces().At(i)->PrintTo(os_);
    }
    *os_ << " {" << newline_;
    PrintVisitor nested = Indent();
    for (int i = 0; i < type->Members().Size(); ++i) {
      PutIndent(depth_ + 1);
      type->Members().At(i)->Accept(&nested);
      *os_ << newline_;
    }
    PutIndent(depth_);
    *os_ << '}';
  }

  VISIT_DECL(InterfaceDecl, type) {
    type->Mods().PrintTo(os_);
    *os_ << "interface ";
    *os_ << type->NameToken().TypeInfo();
    for (int i = 0; i < type->Interfaces().Size(); ++i) {
      if (i == 0) {
        *os_ << " extends ";
      } else {
        *os_ << ',' << space_;
      }
      type->Interfaces().At(i)->PrintTo(os_);
    }
    *os_ << " {" << newline_;
    PrintVisitor nested = Indent();
    for (int i = 0; i < type->Members().Size(); ++i) {
      PutIndent(depth_ + 1);
      type->Members().At(i)->Accept(&nested);
      *os_ << newline_;
    }
    PutIndent(depth_);
    *os_ << '}';
  }

  VISIT_DECL(ImportDecl, import) {
    *os_ << "import ";
    import->Name()->PrintTo(os_);
    *os_ << " ;";
  }

  VISIT_DECL(CompUnit, unit) {
    if (unit->Package() != nullptr) {
      *os_ << "package ";
      unit->Package()->PrintTo(os_);
      *os_ << ";" << newline_;
    }

    for (int i = 0; i < unit->Imports().Size(); ++i) {
      *os_ << "import ";
      unit->Imports().At(i)->Name()->PrintTo(os_);
      if (unit->Imports().At(i)->IsWildCard()) {
        *os_ << ".*";
      }
      *os_ << ";" << newline_;
    }

    for (int i = 0; i < unit->Types().Size(); ++i) {
      unit->Types().At(i)->Accept(this);
      *os_ << newline_;
    }
  }

  VISIT_DECL(Program, prog) {
    // TODO: print the file name?
    for (int i = 0; i < prog->CompUnits().Size(); ++i) {
      prog->CompUnits().At(i)->Accept(this);
    }
  }

 private:
  PrintVisitor(std::ostream* os, int depth, const string& newline,
               const string& tab, const string& space)
      : Visitor(),
        os_(os),
        depth_(depth),
        newline_(newline),
        tab_(tab),
        space_(space) {}

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

}  // namespace parser

#endif
