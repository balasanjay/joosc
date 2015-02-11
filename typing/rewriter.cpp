
#include "typing/rewriter.h"
#include "lexer/lexer.h"
#include "parser/ast.h"

namespace parser {

using lexer::Token;
using base::Error;
using base::FileSet;

REWRITE_DEFN(Rewriter, ArrayIndexExpr, Expr, expr) {
  Expr* base = expr.Base().AcceptRewriter(this);
  Expr* index = expr.Index().AcceptRewriter(this);
  return new ArrayIndexExpr(base, index);
}
REWRITE_DEFN(Rewriter, BinExpr, Expr, expr) {
  Expr* lhs = expr.Lhs().AcceptRewriter(this);
  Expr* rhs = expr.Rhs().AcceptRewriter(this);
  return new BinExpr(lhs, expr.Op(), rhs);
}
REWRITE_DEFN(Rewriter, CallExpr, Expr, expr) {
  Expr* base = expr.Base().AcceptRewriter(this);
  uptr<ArgumentList> args(expr.Args().AcceptRewriter(this));
  return new CallExpr(base, expr.Lparen(), std::move(*args));
}
REWRITE_DEFN(Rewriter, CastExpr, Expr, expr) {
  Type* type = expr.GetType().clone();
  Expr* castedExpr = expr.GetExpr().AcceptRewriter(this);
  return new CastExpr(type, castedExpr);
}
REWRITE_DEFN(Rewriter, FieldDerefExpr, Expr, expr) {
  Expr* base = expr.Base().AcceptRewriter(this);
  return new FieldDerefExpr(base, expr.FieldName(), expr.GetToken());
}
REWRITE_DEFN(Rewriter, BoolLitExpr, Expr, expr) {
  return new BoolLitExpr(expr.GetToken());
}
REWRITE_DEFN(Rewriter, CharLitExpr, Expr, expr) {
  return new CharLitExpr(expr.GetToken());
}
REWRITE_DEFN(Rewriter, StringLitExpr, Expr, expr) {
  return new StringLitExpr(expr.GetToken());
}
REWRITE_DEFN(Rewriter, NullLitExpr, Expr, expr) {
  return new NullLitExpr(expr.GetToken());
}
REWRITE_DEFN(Rewriter, IntLitExpr, Expr, expr) {
  return new IntLitExpr(expr.GetToken(), expr.Value());
}
REWRITE_DEFN(Rewriter, NameExpr, Expr, expr) {
  return new NameExpr(expr.Name());
}
REWRITE_DEFN(Rewriter, NewArrayExpr, Expr, expr) {
  Type* type = expr.GetType().clone();
  Expr* arrayExpr = nullptr;
  if (expr.GetExpr() != nullptr) {
    arrayExpr = expr.GetExpr()->AcceptRewriter(this);
  }
  return new NewArrayExpr(type, arrayExpr);
}
REWRITE_DEFN(Rewriter, NewClassExpr, Expr, expr) {
  Type* type = expr.GetType().clone();
  uptr<ArgumentList> args(expr.Args().AcceptRewriter(this));
  return new NewClassExpr(expr.NewToken(), type, std::move(*args));
}
REWRITE_DEFN(Rewriter, ParenExpr, Expr, expr) {
  return new ParenExpr(expr.Nested().AcceptRewriter(this));
}
REWRITE_DEFN(Rewriter, ThisExpr, Expr, ) {
  return new ThisExpr();
}
REWRITE_DEFN(Rewriter, UnaryExpr, Expr, expr) {
  return new UnaryExpr(expr.Op(), expr.Rhs().AcceptRewriter(this));
}
REWRITE_DEFN(Rewriter, InstanceOfExpr, Expr, expr) {
  Expr* lhs = expr.Lhs().AcceptRewriter(this);
  Type* type = expr.GetType().clone();
  return new InstanceOfExpr(lhs, expr.InstanceOf(), type);
}

REWRITE_DEFN(Rewriter, BlockStmt, Stmt, stmt) {
  base::UniquePtrVector<Stmt> newStmts;
  const auto& stmts = stmt.Stmts();
  for (int i = 0; i < stmts.Size(); ++i) {
    newStmts.Append(stmts.At(i)->AcceptRewriter(this));
  }
  return new BlockStmt(std::move(newStmts));
}
REWRITE_DEFN(Rewriter, EmptyStmt, Stmt, ) {
  return new EmptyStmt();
}
REWRITE_DEFN(Rewriter, ExprStmt, Stmt, stmt) {
  return new ExprStmt(stmt.GetExpr().AcceptRewriter(this));
}
REWRITE_DEFN(Rewriter, LocalDeclStmt, Stmt, stmt) {
  Type* type = stmt.GetType().clone();
  Expr* expr = stmt.GetExpr().AcceptRewriter(this);
  return new LocalDeclStmt(type, stmt.Ident(), expr);
}
REWRITE_DEFN(Rewriter, ReturnStmt, Stmt, stmt) {
  Expr* expr = nullptr;
  if (stmt.GetExpr() != nullptr) {
    expr = stmt.GetExpr()->AcceptRewriter(this);
  }
  return new ReturnStmt(expr);
}
REWRITE_DEFN(Rewriter, IfStmt, Stmt, stmt) {
  Expr* cond = stmt.Cond().AcceptRewriter(this);
  Stmt* trueBody = stmt.TrueBody().AcceptRewriter(this);
  Stmt* falseBody = stmt.FalseBody().AcceptRewriter(this);
  return new IfStmt(cond, trueBody, falseBody);
}
REWRITE_DEFN(Rewriter, ForStmt, Stmt, stmt) {
  Stmt* init = stmt.Init().AcceptRewriter(this);
  Expr* cond = nullptr;
  if (stmt.Cond() != nullptr) {
    cond = stmt.Cond()->AcceptRewriter(this);
  }
  Expr* update = nullptr;
  if (stmt.Update() != nullptr) {
    update = stmt.Update()->AcceptRewriter(this);
  }
  Stmt* body = stmt.Body().AcceptRewriter(this);
  return new ForStmt(init, cond, update, body);
}

REWRITE_DEFN(Rewriter, WhileStmt, Stmt, stmt) {
  Expr* cond = stmt.Cond().AcceptRewriter(this);
  Stmt* body = stmt.Body().AcceptRewriter(this);
  return new WhileStmt(cond, body);
}

REWRITE_DEFN(Rewriter, ArgumentList, ArgumentList, args) {
  base::UniquePtrVector<Expr> newExprs;
  for (int i = 0; i < args.Args().Size(); ++i) {
    newExprs.Append(args.Args().At(i)->AcceptRewriter(this));
  }
  return new ArgumentList(std::move(newExprs));
}

REWRITE_DEFN(Rewriter, ParamList, ParamList, params) {
  base::UniquePtrVector<Param> newParams;
  for (int i = 0; i < params.Params().Size(); ++i) {
    newParams.Append(params.Params().At(i)->AcceptRewriter(this));
  }
  return new ParamList(std::move(newParams));
}

REWRITE_DEFN(Rewriter, Param, Param, param) {
  Type* type = param.GetType().clone();
  return new Param(type, param.Ident());
}

REWRITE_DEFN(Rewriter, FieldDecl, MemberDecl, field) {
  ModifierList mods(field.Mods());
  Type* type = field.GetType().clone();
  Expr* val = nullptr;
  if (field.Val() != nullptr) {
    val = field.Val()->AcceptRewriter(this);
  }
  return new FieldDecl(std::move(mods), type, field.Ident(), val);
}

REWRITE_DEFN(Rewriter, ConstructorDecl, MemberDecl, meth) {
  ModifierList mods(meth.Mods());
  uptr<ParamList> params(meth.Params().AcceptRewriter(this));
  Stmt* body = meth.Body().AcceptRewriter(this);
  return new ConstructorDecl(std::move(mods), meth.Ident(), std::move(*params), body);
}

REWRITE_DEFN(Rewriter, MethodDecl, MemberDecl, meth) {
  ModifierList mods(meth.Mods());
  Type* type = meth.GetType().clone();
  uptr<ParamList> params(meth.Params().AcceptRewriter(this));
  Stmt* body = meth.Body().AcceptRewriter(this);
  return new MethodDecl(std::move(mods), type, meth.Ident(), std::move(*params), body);
}

REWRITE_DEFN(Rewriter, ClassDecl, TypeDecl, type) {
  ModifierList mods(type.Mods());
  base::UniquePtrVector<ReferenceType> interfaces;
  for (int i = 0; i < type.Interfaces().Size(); ++i) {
    interfaces.Append(static_cast<ReferenceType*>(type.Interfaces().At(i)->clone()));
  }
  base::UniquePtrVector<MemberDecl> members;
  for (int i = 0; i < type.Members().Size(); ++i) {
    members.Append(type.Members().At(i)->AcceptRewriter(this));
  }
  ReferenceType* super = nullptr;
  if (type.Super() != nullptr) {
    super = static_cast<ReferenceType*>(type.Super()->clone());
  }
  return new ClassDecl(std::move(mods), type.Name(), type.NameToken(), std::move(interfaces), std::move(members), super);
}

REWRITE_DEFN(Rewriter, InterfaceDecl, TypeDecl, type) {
  ModifierList mods(type.Mods());
  base::UniquePtrVector<ReferenceType> interfaces;
  for (int i = 0; i < type.Interfaces().Size(); ++i) {
    interfaces.Append(static_cast<ReferenceType*>(type.Interfaces().At(i)->clone()));
  }
  base::UniquePtrVector<MemberDecl> members;
  for (int i = 0; i < type.Members().Size(); ++i) {
    members.Append(type.Members().At(i)->AcceptRewriter(this));
  }
  return new InterfaceDecl(std::move(mods), type.Name(), type.NameToken(), std::move(interfaces), std::move(members));
}

REWRITE_DEFN(Rewriter, CompUnit, CompUnit, unit) {
  QualifiedName* package = nullptr;
  if (unit.Package() != nullptr) {
    package = new QualifiedName(*unit.Package());
  }

  base::UniquePtrVector<TypeDecl> decls;
  for (int i = 0; i < unit.Types().Size(); ++i) {
    decls.Append(unit.Types().At(i)->AcceptRewriter(this));
  }
  return new CompUnit(package, unit.Imports(), std::move(decls));
}

REWRITE_DEFN(Rewriter, Program, Program, prog) {
  base::UniquePtrVector<CompUnit> units;
  for (int i = 0; i < prog.CompUnits().Size(); ++i) {
    units.Append(prog.CompUnits().At(i)->AcceptRewriter(this));
  }
  return new Program(std::move(units));
}

}  // namespace parser

