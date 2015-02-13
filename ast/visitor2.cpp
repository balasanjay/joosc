#include "ast/visitor2.h"

#include "ast/ast.h"
#include "lexer/lexer.h"

namespace ast {

using lexer::Token;
using base::Error;
using base::FileSet;
using base::SharedPtrVector;

#define SHORT_CIRCUIT(type, var) \
  {                                    \
    auto result = Visit##type(var); \
    if (result == VisitResult::SKIP) { \
      return var##ptr; \
    } \
    if (result == VisitResult::PRUNE) { \
      return nullptr; \
    } \
  }

REWRITE_DEFN2(Visitor2, ArrayIndexExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(ArrayIndexExpr, expr);

  sptr<Expr> base = Visit(this, expr.BasePtr());
  sptr<Expr> index = Visit(this, expr.IndexPtr());
  if (base == nullptr || index == nullptr) {
    return nullptr;
  }
  if (base == expr.BasePtr() && index == expr.IndexPtr()) {
    return exprptr;
  }
  return make_shared<ArrayIndexExpr>(base, index);
}

REWRITE_DEFN2(Visitor2, BinExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(BinExpr, expr);

  sptr<Expr> lhs = Visit(this, expr.LhsPtr());
  sptr<Expr> rhs = Visit(this, expr.RhsPtr());
  if (lhs == nullptr || rhs == nullptr) {
    return nullptr;
  }
  if (lhs == expr.LhsPtr() && rhs == expr.RhsPtr()) {
    return exprptr;
  }

  return make_shared<BinExpr>(lhs, expr.Op(), rhs);
}

REWRITE_DEFN2(Visitor2, CallExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CallExpr, expr);

  bool argsChanged = false;
  sptr<Expr> base = Visit(this, expr.BasePtr());
  SharedPtrVector<Expr> args = AcceptMulti(expr.Args(), &argsChanged);

  if (base == nullptr || args.Size() != expr.Args().Size()) {
    return nullptr;
  }
  if (base == expr.BasePtr() && !argsChanged) {
    return exprptr;
  }

  return make_shared<CallExpr>(base, expr.Lparen(), args);
}

REWRITE_DEFN2(Visitor2, CastExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CastExpr, expr);

  sptr<Expr> castedExpr = Visit(this, expr.GetExprPtr());
  if (castedExpr == nullptr) {
    return nullptr;
  } else if (castedExpr == expr.GetExprPtr()) {
    return exprptr;
  }
  return make_shared<CastExpr>(expr.GetTypePtr(), castedExpr);
}

REWRITE_DEFN2(Visitor2, FieldDerefExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(FieldDerefExpr, expr);
  sptr<Expr> base = Visit(this, expr.BasePtr());
  if (base == nullptr) {
    return nullptr;
  } else if (base == expr.BasePtr()) {
    return exprptr;
  }
  return make_shared<FieldDerefExpr>(base, expr.FieldName(), expr.GetToken());
}

REWRITE_DEFN2(Visitor2, BoolLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(BoolLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, CharLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CharLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, StringLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(StringLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, NullLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NullLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, IntLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(IntLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, NameExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NameExpr, expr);
  return exprptr;
}

REWRITE_DEFN2(Visitor2, NewArrayExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NewArrayExpr, expr);

  sptr<Expr> arrayExpr = nullptr;
  if (expr.GetExprPtr() != nullptr) {
    arrayExpr = Visit(this, expr.GetExprPtr());
  }

  // We don't prune the subtree if the expr returns null, because the expr is a
  // nullable field.

  if (arrayExpr == expr.GetExprPtr()) {
    return exprptr;
  }
  return make_shared<NewArrayExpr>(expr.GetTypePtr(), arrayExpr);
}

REWRITE_DEFN2(Visitor2, NewClassExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NewClassExpr, expr);

  bool argsChanged = false;
  SharedPtrVector<Expr> args = AcceptMulti(expr.Args(), &argsChanged);

  if (args.Size() != expr.Args().Size()) {
    return nullptr;
  }
  if (!argsChanged) {
    return exprptr;
  }
  return make_shared<NewClassExpr>(expr.NewToken(), expr.GetTypePtr(), args);
}

REWRITE_DEFN2(Visitor2, ParenExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(ParenExpr, expr);
  sptr<Expr> nested = Visit(this, expr.NestedPtr());
  if (nested == nullptr) {
    return nullptr;
  } else if (nested == expr.NestedPtr()) {
    return exprptr;
  }
  return make_shared<ParenExpr>(nested);
}

REWRITE_DEFN2(Visitor2, ThisExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(ThisExpr, expr);
  return exprptr;
}

REWRITE_DEFN2(Visitor2, UnaryExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(UnaryExpr, expr);
  sptr<Expr> rhs = Visit(this, expr.RhsPtr());
  if (rhs == nullptr) {
    return nullptr;
  } else if (rhs == expr.RhsPtr()) {
    return exprptr;
  }
  return make_shared<UnaryExpr>(expr.Op(), rhs);
}

REWRITE_DEFN2(Visitor2, InstanceOfExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(InstanceOfExpr, expr);
  sptr<Expr> lhs = Visit(this, expr.LhsPtr());
  if (lhs == nullptr) {
    return nullptr;
  } else if (lhs == expr.LhsPtr()) {
    return exprptr;
  }
  return make_shared<InstanceOfExpr>(lhs, expr.InstanceOf(), expr.GetTypePtr());
}

REWRITE_DEFN2(Visitor2, BlockStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(BlockStmt, stmt);

  bool stmtsChanged = false;
  SharedPtrVector<Stmt> newStmts = AcceptMulti(stmt.Stmts(), &stmtsChanged);

  if (!stmtsChanged) {
    return stmtptr;
  }

  return make_shared<BlockStmt>(newStmts);
}

REWRITE_DEFN2(Visitor2, EmptyStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(EmptyStmt, stmt);
  return stmtptr;
}

REWRITE_DEFN2(Visitor2, ExprStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(ExprStmt, stmt);

  sptr<Expr> expr = Visit(this, stmt.GetExprPtr());
  if (expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<ExprStmt>(expr);
}

REWRITE_DEFN2(Visitor2, LocalDeclStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(LocalDeclStmt, stmt);

  sptr<Expr> expr = Visit(this, stmt.GetExprPtr());
  if (expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<LocalDeclStmt>(stmt.GetTypePtr(), stmt.Ident(), expr);
}

REWRITE_DEFN2(Visitor2, ReturnStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(ReturnStmt, stmt);
  if (stmt.GetExprPtr() == nullptr) {
    return stmtptr;
  }

  sptr<Expr> expr = Visit(this, stmt.GetExprPtr());
  if (expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<ReturnStmt>(expr);
}

REWRITE_DEFN2(Visitor2, IfStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(IfStmt, stmt);

  sptr<Expr> cond = Visit(this, stmt.CondPtr());
  if (cond == nullptr) {
    return nullptr;
  }

  sptr<Stmt> trueBody = Visit(this, stmt.TrueBodyPtr());
  sptr<Stmt> falseBody = Visit(this, stmt.FalseBodyPtr());

  // If a subtree was pruned, then make it an empty statement.
  if (trueBody == nullptr) {
    trueBody = make_shared<EmptyStmt>();
  }
  if (falseBody == nullptr) {
    falseBody = make_shared<EmptyStmt>();
  }

  if (cond == stmt.CondPtr() && trueBody == stmt.TrueBodyPtr() && falseBody == stmt.FalseBodyPtr()) {
    return stmtptr;
  }

  return make_shared<IfStmt>(cond, trueBody, falseBody);
}

REWRITE_DEFN2(Visitor2, ForStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(ForStmt, stmt);

  sptr<Stmt> init = Visit(this, stmt.InitPtr());

  sptr<Expr> cond = nullptr;
  if (stmt.CondPtr() != nullptr) {
    cond = Visit(this, stmt.CondPtr());
  }

  sptr<Expr> update = nullptr;
  if (stmt.UpdatePtr() != nullptr) {
    update = Visit(this, stmt.UpdatePtr());
  }

  sptr<Stmt> body = Visit(this, stmt.BodyPtr());
  if (body == nullptr) {
    body = make_shared<EmptyStmt>();
  }

  if (init == nullptr) {
    return nullptr;
  } else if (init == stmt.InitPtr() && cond == stmt.CondPtr() && update == stmt.UpdatePtr() && body == stmt.BodyPtr()) {
    return stmtptr;
  }

  return make_shared<ForStmt>(init, cond, update, body);
}

REWRITE_DEFN2(Visitor2, WhileStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(WhileStmt, stmt);

  sptr<Expr> cond = Visit(this, stmt.CondPtr());
  sptr<Stmt> body = Visit(this, stmt.BodyPtr());

  if (cond == nullptr) {
    return nullptr;
  }
  if (body == nullptr) {
    body = make_shared<EmptyStmt>();
  }

  if (cond == stmt.CondPtr() && body == stmt.BodyPtr()) {
    return stmtptr;
  }

  return make_shared<WhileStmt>(cond, body);
}

REWRITE_DEFN2(Visitor2, ParamList, ParamList, params, paramsptr) {
  SHORT_CIRCUIT(ParamList, params);

  bool paramsChanged = false;
  SharedPtrVector<Param> newParams = AcceptMulti(params.Params(), &paramsChanged);

  if (newParams.Size() != params.Params().Size()) {
    return nullptr;
  }
  if (!paramsChanged) {
    return paramsptr;
  }

  return make_shared<ParamList>(newParams);
}


REWRITE_DEFN2(Visitor2, Param, Param, param, paramptr) {
  SHORT_CIRCUIT(Param, param);
  return paramptr;
}

REWRITE_DEFN2(Visitor2, FieldDecl, MemberDecl, field, fieldptr) {
  SHORT_CIRCUIT(FieldDecl, field);

  sptr<Expr> val;
  if (field.ValPtr() != nullptr) {
    val = Visit(this, field.ValPtr());
  }

  if (val == field.ValPtr()) {
    return fieldptr;
  }

  return make_shared<FieldDecl>(field.Mods(), field.GetTypePtr(), field.Ident(), val);
}

REWRITE_DEFN2(Visitor2, MethodDecl, MemberDecl, meth, methptr) {
  SHORT_CIRCUIT(MethodDecl, meth);

  sptr<ParamList> params = Visit(this, meth.ParamsPtr());
  sptr<Stmt> body = Visit(this, meth.BodyPtr());

  if (params == nullptr || body == nullptr) {
    return nullptr;
  } else if (params == meth.ParamsPtr() && body == meth.BodyPtr()) {
    return methptr;
  }

  return make_shared<MethodDecl>(meth.Mods(), meth.GetTypePtr(), meth.Ident(), params, body);
}


REWRITE_DEFN2(Visitor2, ConstructorDecl, MemberDecl, meth, methptr) {
  SHORT_CIRCUIT(ConstructorDecl, meth);

  sptr<ParamList> params = Visit(this, meth.ParamsPtr());
  sptr<Stmt> body = Visit(this, meth.BodyPtr());

  if (params == nullptr || body == nullptr) {
    return nullptr;
  } else if (params == meth.ParamsPtr() && body == meth.BodyPtr()) {
    return methptr;
  }

  return make_shared<ConstructorDecl>(meth.Mods(), meth.Ident(), params, body);
}

REWRITE_DEFN2(Visitor2, ClassDecl, TypeDecl, type, typeptr) {
  SHORT_CIRCUIT(ClassDecl, type);

  bool membersChanged = false;
  SharedPtrVector<MemberDecl> newMembers = AcceptMulti(type.Members(), &membersChanged);
  if (!membersChanged) {
    return typeptr;
  }

  return make_shared<ClassDecl>(type.Mods(), type.Name(), type.NameToken(), type.Interfaces(), newMembers, type.SuperPtr(), type.GetTypeId());
}

REWRITE_DEFN2(Visitor2, InterfaceDecl, TypeDecl, type, typeptr) {
  SHORT_CIRCUIT(InterfaceDecl, type);

  bool membersChanged = false;
  SharedPtrVector<MemberDecl> newMembers = AcceptMulti(type.Members(), &membersChanged);
  if (!membersChanged) {
    return typeptr;
  }

  return make_shared<InterfaceDecl>(type.Mods(), type.Name(), type.NameToken(), type.Interfaces(), newMembers, type.GetTypeId());
}

REWRITE_DEFN2(Visitor2, CompUnit, CompUnit, unit, unitptr) {
  SHORT_CIRCUIT(CompUnit, unit);

  bool typesChanged = false;
  SharedPtrVector<TypeDecl> newTypes = AcceptMulti(unit.Types(), &typesChanged);

  if (!typesChanged) {
    return unitptr;
  }
  return make_shared<CompUnit>(unit.PackagePtr(), unit.Imports(), newTypes);
}

REWRITE_DEFN2(Visitor2, Program, Program, prog, progptr) {
  SHORT_CIRCUIT(Program, prog);

  bool unitsChanged = false;
  SharedPtrVector<CompUnit> units = AcceptMulti(prog.CompUnits(), &unitsChanged);

  if (!unitsChanged) {
    return progptr;
  }

  return make_shared<Program>(units);
}
}  // namespace ast

