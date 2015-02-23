#ifndef AST_PRINT_VISITOR_H
#define AST_PRINT_VISITOR_H

#include "ast/ast.h"
#include "ast/visitor.h"

namespace ast {

class PrintVisitor final : public Visitor {
 public:
  static PrintVisitor Pretty(std::ostream* os) {
    return PrintVisitor(os, 0, "\n", "  ", " ");
  }

  static PrintVisitor Compact(std::ostream* os) {
    return PrintVisitor(os, 0, "", "", "");
  }

  VISIT_DECL(ArrayIndexExpr, expr) {
    Visit(expr.BasePtr());
    *os_ << '[';
    Visit(expr.IndexPtr());
    *os_ << ']';
    return VisitResult::SKIP;
  }

  VISIT_DECL(BinExpr, expr) {
    *os_ << '(';
    Visit(expr.LhsPtr());
    *os_ << ' ' << expr.Op().TypeInfo() << ' ';
    Visit(expr.RhsPtr());
    *os_ << ')';
    return VisitResult::SKIP;
  }

  VISIT_DECL(CallExpr, expr) {
    Visit(expr.BasePtr());
    *os_ << '(';
    PrintArgList(expr.Args());
    *os_ << ')';
    return VisitResult::SKIP;
  }

  VISIT_DECL(CastExpr, expr) {
    *os_ << "cast<";
    expr.GetType().PrintTo(os_);
    *os_ << ">(";
    Visit(expr.GetExprPtr());
    *os_ << ')';
    return VisitResult::SKIP;
  }

  VISIT_DECL(InstanceOfExpr, expr) {
    *os_ << '(';
    Visit(expr.LhsPtr());
    *os_ << " instanceof ";
    expr.GetType().PrintTo(os_);
    *os_ << ')';
    return VisitResult::SKIP;
  }

  VISIT_DECL(FieldDerefExpr, expr) {
    Visit(expr.BasePtr());
    *os_ << '.' << expr.FieldName();
    return VisitResult::SKIP;
  }

  VISIT_DECL(BoolLitExpr, expr) {
    *os_ << expr.GetToken().TypeInfo();
    return VisitResult::SKIP;
  }

  VISIT_DECL(StringLitExpr, expr) {
    *os_ << expr.GetToken().TypeInfo();
    return VisitResult::SKIP;
  }

  VISIT_DECL(CharLitExpr, expr) {
    *os_ << expr.GetToken().TypeInfo();
    return VisitResult::SKIP;
  }

  VISIT_DECL(NullLitExpr, expr) {
    *os_ << expr.GetToken().TypeInfo();
    return VisitResult::SKIP;
  }

  VISIT_DECL(IntLitExpr, expr) {
    *os_ << expr.GetToken().TypeInfo();
    return VisitResult::SKIP;
  }

  VISIT_DECL(NameExpr, expr) {
    *os_ << expr.Name().Name();
    return VisitResult::SKIP;
  }

  VISIT_DECL(NewArrayExpr, expr) {
    *os_ << "new<array<";
    expr.GetType().PrintTo(os_);

    *os_ << ">>(";
    if (expr.GetExprPtr() != nullptr) {
      Visit(expr.GetExprPtr());
    }
    *os_ << ")";
    return VisitResult::SKIP;
  }

  VISIT_DECL(NewClassExpr, expr) {
    *os_ << "new<";
    expr.GetType().PrintTo(os_);
    *os_ << ">(";
    PrintArgList(expr.Args());
    *os_ << ")";
    return VisitResult::SKIP;
  }

  VISIT_DECL(ParenExpr, expr) {
    *os_ << "(";
    Visit(expr.NestedPtr());
    *os_ << ")";
    return VisitResult::SKIP;
  }

  VISIT_DECL(ThisExpr, ) {
    *os_ << "this";
    return VisitResult::SKIP;
  }

  VISIT_DECL(UnaryExpr, expr) {
    *os_ << '(' << expr.Op().TypeInfo() << ' ';
    Visit(expr.RhsPtr());
    *os_ << ')';
    return VisitResult::SKIP;
  }

  VISIT_DECL(BlockStmt, stmt) {
    *os_ << "{" << newline_;

    PrintVisitor nested = Indent();
    for (const auto& substmt : stmt.Stmts().Vec()) {
      PutIndent(depth_ + 1);
      Visit(substmt);
      *os_ << newline_;
    }

    PutIndent(depth_);
    *os_ << '}';
    return VisitResult::SKIP;
  }

  VISIT_DECL(EmptyStmt, ) {
    *os_ << ';';
    return VisitResult::SKIP;
  }

  VISIT_DECL(ExprStmt, stmt) {
    Visit(stmt.GetExprPtr());
    *os_ << ';';
    return VisitResult::SKIP;
  }

  VISIT_DECL(LocalDeclStmt, stmt) {
    stmt.GetType().PrintTo(os_);
    *os_ << ' ' << stmt.Name() << space_ << '=' << space_;
    Visit(stmt.GetExprPtr());
    *os_ << ';';
    return VisitResult::SKIP;
  }

  VISIT_DECL(ReturnStmt, stmt) {
    *os_ << "return";
    if (stmt.GetExprPtr() != nullptr) {
      *os_ << ' ';
      Visit(stmt.GetExprPtr());
    }
    *os_ << ';';
    return VisitResult::SKIP;
  }

  VISIT_DECL(IfStmt, stmt) {
    *os_ << "if" << space_ << '(';
    Visit(stmt.CondPtr());
    *os_ << ')' << space_ << '{';
    Visit(stmt.TrueBodyPtr());
    *os_ << '}' << space_ << "else" << space_ << '{';
    Visit(stmt.FalseBodyPtr());
    *os_ << '}';
    return VisitResult::SKIP;
  }

  VISIT_DECL(ForStmt, stmt) {
    *os_ << "for" << space_ << '(';
    Visit(stmt.InitPtr());
    if (stmt.CondPtr() != nullptr) {
      *os_ << space_;
      Visit(stmt.CondPtr());
    }
    *os_ << ';';
    if (stmt.UpdatePtr() != nullptr) {
      *os_ << space_;
      Visit(stmt.UpdatePtr());
    }
    *os_ << ')' << space_ << '{';
    Visit(stmt.BodyPtr());
    *os_ << '}';
    return VisitResult::SKIP;
  }

  VISIT_DECL(WhileStmt, stmt) {
    *os_ << "while" << space_ << '(';
    Visit(stmt.CondPtr());
    *os_ << ')' << space_ << '{';
    Visit(stmt.BodyPtr());
    *os_ << '}';
    return VisitResult::SKIP;
  }

  VISIT_DECL(ParamList, params) {
    for (int i = 0; i < params.Params().Size(); ++i) {
      if (i > 0) {
        *os_ << ',' << space_;
      }
      Visit(params.Params().At(i));
    }
    return VisitResult::SKIP;
  }

  VISIT_DECL(Param, param) {
    param.GetType().PrintTo(os_);
    *os_ << ' ' << param.Name();
    return VisitResult::SKIP;
  }

  VISIT_DECL(FieldDecl, field) {
    field.Mods().PrintTo(os_);
    field.GetType().PrintTo(os_);
    *os_ << ' ';
    *os_ << field.Name();
    if (field.ValPtr() != nullptr) {
      *os_ << space_ << '=' << space_;
      Visit(field.ValPtr());
    }
    *os_ << ';';
    return VisitResult::SKIP;
  }

  VISIT_DECL(MethodDecl, meth) {
    meth.Mods().PrintTo(os_);
    if (meth.TypePtr() != nullptr) {
      meth.TypePtr()->PrintTo(os_);
      *os_ << ' ';
    }
    *os_ << meth.Name();
    *os_ << '(';
    Visit(meth.ParamsPtr());
    *os_ << ')' << space_;
    Visit(meth.BodyPtr());
    return VisitResult::SKIP;
  }

  VISIT_DECL(TypeDecl, type) {
    type.Mods().PrintTo(os_);
    if (type.Kind() == TypeKind::CLASS) {
      *os_ << "class ";
    } else {
      *os_ << "interface ";
    }
    *os_ << type.Name();

    auto printNameList = [&](const string& label, const vector<QualifiedName>& elems) {
      bool first = true;
      for (const auto& elem : elems) {
        if (first) {
          *os_ << ' ' << label << ' ';
        } else {
          *os_ << ',' << space_;
        }
        first = false;
        elem.PrintTo(os_);
      }
    };
    printNameList("extends", type.Extends());
    printNameList("implements", type.Implements());

    *os_ << " {" << newline_;
    PrintVisitor nested = Indent();
    for (int i = 0; i < type.Members().Size(); ++i) {
      PutIndent(depth_ + 1);
      nested.Visit(type.Members().At(i));
      *os_ << newline_;
    }
    PutIndent(depth_);
    *os_ << '}';
    return VisitResult::SKIP;
  }

  VISIT_DECL(CompUnit, unit) {
    if (unit.PackagePtr() != nullptr) {
      *os_ << "package ";
      unit.PackagePtr()->PrintTo(os_);
      *os_ << ";" << newline_;
    }

    for (const auto& import : unit.Imports()) {
      *os_ << "import ";
      import.Name().PrintTo(os_);
      if (import.IsWildCard()) {
        *os_ << ".*";
      }
      *os_ << ";" << newline_;
    }

    for (int i = 0; i < unit.Types().Size(); ++i) {
      Visit(unit.Types().At(i));
      *os_ << newline_;
    }
    return VisitResult::SKIP;
  }

  VISIT_DECL(Program, prog) {
    // TODO: print the file name?
    for (int i = 0; i < prog.CompUnits().Size(); ++i) {
      Visit(prog.CompUnits().At(i));
    }
    return VisitResult::SKIP;
  }

 private:
  PrintVisitor(std::ostream* os, int depth, const string& newline,
               const string& tab, const string& space)
      : os_(os),
        depth_(depth),
        newline_(newline),
        tab_(tab),
        space_(space) {}

  PrintVisitor Indent() const {
    return PrintVisitor(os_, depth_ + 1, newline_, tab_, space_);
  }

  void PrintArgList(const base::SharedPtrVector<const Expr>& args) {
    bool first = true;
    for (const auto& arg : args.Vec()) {
      if (!first) {
        *os_ << ',' << space_;
      }
      first = false;
      Visit(arg);
    }
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

}  // namespace ast

#endif
