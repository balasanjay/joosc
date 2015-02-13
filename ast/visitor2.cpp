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

REWRITE_DEFN2(Visitor2, ArrayIndexExpr, Expr, expr) {
  SHORT_CIRCUIT(ArrayIndexExpr, expr);

  sptr<Expr> base = expr.Base().Accept2(this, expr.BasePtr());
  sptr<Expr> index = expr.Index().Accept2(this, expr.IndexPtr());
  if (base == nullptr || index == nullptr) {
    return nullptr;
  }
  if (base == expr.BasePtr() && index == expr.IndexPtr()) {
    return exprptr;
  }
  return make_shared<ArrayIndexExpr>(base, index);
}

REWRITE_DEFN2(Visitor2, BinExpr, Expr, expr) {
  SHORT_CIRCUIT(BinExpr, expr);

  sptr<Expr> lhs = expr.Lhs().Accept2(this, expr.LhsPtr());
  sptr<Expr> rhs = expr.Rhs().Accept2(this, expr.RhsPtr());
  if (lhs == nullptr || rhs == nullptr) {
    return nullptr;
  }
  if (lhs == expr.LhsPtr() && rhs == expr.RhsPtr()) {
    return exprptr;
  }

  return make_shared<BinExpr>(lhs, expr.Op(), rhs);
}

REWRITE_DEFN2(Visitor2, CallExpr, Expr, expr) {
  SHORT_CIRCUIT(CallExpr, expr);

  bool argsChanged = false;
  sptr<Expr> base = expr.Base().Accept2(this, expr.BasePtr());
  SharedPtrVector<Expr> args = AcceptMulti(expr.Args(), &argsChanged);

  if (base == nullptr || args.Size() != expr.Args().Size()) {
    return nullptr;
  }
  if (base == expr.BasePtr() && !argsChanged) {
    return exprptr;
  }

  return make_shared<CallExpr>(base, expr.Lparen(), args);
}

REWRITE_DEFN2(Visitor2, CastExpr, Expr, expr) {
  SHORT_CIRCUIT(CastExpr, expr);

  sptr<Expr> castedExpr = expr.GetExpr().Accept2(this, expr.GetExprPtr());
  if (castedExpr == nullptr) {
    return nullptr;
  } else if (castedExpr == expr.GetExprPtr()) {
    return exprptr;
  }
  return make_shared<CastExpr>(expr.GetTypePtr(), castedExpr);
}

REWRITE_DEFN2(Visitor2, FieldDerefExpr, Expr, expr) {
  SHORT_CIRCUIT(FieldDerefExpr, expr);
  sptr<Expr> base = expr.Base().Accept2(this, expr.BasePtr());
  if (base == nullptr) {
    return nullptr;
  } else if (base == expr.BasePtr()) {
    return exprptr;
  }
  return make_shared<FieldDerefExpr>(base, expr.FieldName(), expr.GetToken());
}

REWRITE_DEFN2(Visitor2, BoolLitExpr, Expr, expr) {
  SHORT_CIRCUIT(BoolLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, CharLitExpr, Expr, expr) {
  SHORT_CIRCUIT(CharLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, StringLitExpr, Expr, expr) {
  SHORT_CIRCUIT(StringLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, NullLitExpr, Expr, expr) {
  SHORT_CIRCUIT(NullLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, IntLitExpr, Expr, expr) {
  SHORT_CIRCUIT(IntLitExpr, expr);
  return exprptr;
}
REWRITE_DEFN2(Visitor2, NameExpr, Expr, expr) {
  SHORT_CIRCUIT(NameExpr, expr);
  return exprptr;
}

REWRITE_DEFN2(Visitor2, NewArrayExpr, Expr, expr) {
  SHORT_CIRCUIT(NewArrayExpr, expr);

  sptr<Expr> arrayExpr = nullptr;
  if (expr.GetExprPtr() != nullptr) {
    arrayExpr = expr.GetExprPtr()->Accept2(this, expr.GetExprPtr());
  }

  // We don't prune the subtree if the expr returns null, because the expr is a
  // nullable field.

  if (arrayExpr == expr.GetExprPtr()) {
    return exprptr;
  }
  return make_shared<NewArrayExpr>(expr.GetTypePtr(), arrayExpr);
}

REWRITE_DEFN2(Visitor2, NewClassExpr, Expr, expr) {
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

REWRITE_DEFN2(Visitor2, ParenExpr, Expr, expr) {
  SHORT_CIRCUIT(ParenExpr, expr);
  sptr<Expr> nested = expr.Nested().Accept2(this, expr.NestedPtr());
  if (nested == nullptr) {
    return nullptr;
  } else if (nested == expr.NestedPtr()) {
    return exprptr;
  }
  return make_shared<ParenExpr>(nested);
}

REWRITE_DEFN2(Visitor2, ThisExpr, Expr, expr) {
  SHORT_CIRCUIT(ThisExpr, expr);
  return exprptr;
}

REWRITE_DEFN2(Visitor2, UnaryExpr, Expr, expr) {
  SHORT_CIRCUIT(UnaryExpr, expr);
  sptr<Expr> rhs = expr.Rhs().Accept2(this, expr.RhsPtr());
  if (rhs == nullptr) {
    return nullptr;
  } else if (rhs == expr.RhsPtr()) {
    return exprptr;
  }
  return make_shared<UnaryExpr>(expr.Op(), rhs);
}

REWRITE_DEFN2(Visitor2, InstanceOfExpr, Expr, expr) {
  SHORT_CIRCUIT(InstanceOfExpr, expr);
  sptr<Expr> lhs = expr.Lhs().Accept2(this, expr.LhsPtr());
  if (lhs == nullptr) {
    return nullptr;
  } else if (lhs == expr.LhsPtr()) {
    return exprptr;
  }
  return make_shared<InstanceOfExpr>(lhs, expr.InstanceOf(), expr.GetTypePtr());
}

REWRITE_DEFN2(Visitor2, BlockStmt, Stmt, stmt) {
  SHORT_CIRCUIT(BlockStmt, stmt);

  bool stmtsChanged = false;
  SharedPtrVector<Stmt> newStmts = AcceptMulti(stmt.Stmts(), &stmtsChanged);

  if (!stmtsChanged) {
    return stmtptr;
  }

  return make_shared<BlockStmt>(newStmts);
}

REWRITE_DEFN2(Visitor2, EmptyStmt, Stmt, stmt) {
  SHORT_CIRCUIT(EmptyStmt, stmt);
  return stmtptr;
}

REWRITE_DEFN2(Visitor2, ExprStmt, Stmt, stmt) {
  SHORT_CIRCUIT(ExprStmt, stmt);

  sptr<Expr> expr = stmt.GetExpr().Accept2(this, stmt.GetExprPtr());
  if (expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<ExprStmt>(expr);
}

REWRITE_DEFN2(Visitor2, LocalDeclStmt, Stmt, stmt) {
  SHORT_CIRCUIT(LocalDeclStmt, stmt);

  sptr<Expr> expr = stmt.GetExpr().Accept2(this, stmt.GetExprPtr());
  if (expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<LocalDeclStmt>(stmt.GetTypePtr(), stmt.Ident(), expr);
}

REWRITE_DEFN2(Visitor2, ReturnStmt, Stmt, stmt) {
  SHORT_CIRCUIT(ReturnStmt, stmt);
  if (stmt.GetExprPtr() == nullptr) {
    return stmtptr;
  }

  sptr<Expr> expr = stmt.GetExprPtr()->Accept2(this, stmt.GetExprPtr());
  if (expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<ReturnStmt>(expr);
}

REWRITE_DEFN2(Visitor2, IfStmt, Stmt, stmt) {
  SHORT_CIRCUIT(IfStmt, stmt);

  sptr<Expr> cond = stmt.Cond().Accept2(this, stmt.CondPtr());
  if (cond == nullptr) {
    return nullptr;
  }

  sptr<Stmt> trueBody = stmt.TrueBody().Accept2(this, stmt.TrueBodyPtr());
  sptr<Stmt> falseBody = stmt.FalseBody().Accept2(this, stmt.FalseBodyPtr());

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

REWRITE_DEFN2(Visitor2, ForStmt, Stmt, stmt) {
  SHORT_CIRCUIT(ForStmt, stmt);

  sptr<Stmt> init = stmt.Init().Accept2(this, stmt.InitPtr());

  sptr<Expr> cond = nullptr;
  if (stmt.CondPtr() != nullptr) {
    cond = stmt.CondPtr()->Accept2(this, stmt.CondPtr());
  }

  sptr<Expr> update = nullptr;
  if (stmt.UpdatePtr() != nullptr) {
    update = stmt.UpdatePtr()->Accept2(this, stmt.UpdatePtr());
  }

  sptr<Stmt> body = stmt.Body().Accept2(this, stmt.BodyPtr());
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

REWRITE_DEFN2(Visitor2, WhileStmt, Stmt, stmt) {
  SHORT_CIRCUIT(WhileStmt, stmt);

  sptr<Expr> cond = stmt.Cond().Accept2(this, stmt.CondPtr());
  sptr<Stmt> body = stmt.Body().Accept2(this, stmt.BodyPtr());

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

REWRITE_DEFN2(Visitor2, ParamList, ParamList, params) {
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


REWRITE_DEFN2(Visitor2, Param, Param, param) {
  SHORT_CIRCUIT(Param, param);
  return paramptr;
}

REWRITE_DEFN2(Visitor2, FieldDecl, MemberDecl, field) {
  SHORT_CIRCUIT(FieldDecl, field);

  sptr<Expr> val;
  if (field.ValPtr() != nullptr) {
    val = field.ValPtr()->Accept2(this, field.ValPtr());
  }

  if (val == field.ValPtr()) {
    return fieldptr;
  }

  return make_shared<FieldDecl>(field.Mods(), field.GetTypePtr(), field.Ident(), val);
}

REWRITE_DEFN2(Visitor2, MethodDecl, MemberDecl, meth) {
  SHORT_CIRCUIT(MethodDecl, meth);

  sptr<ParamList> params = meth.Params().Accept2(this, meth.ParamsPtr());
  sptr<Stmt> body = meth.Body().Accept2(this, meth.BodyPtr());

  if (params == nullptr || body == nullptr) {
    return nullptr;
  } else if (params == meth.ParamsPtr() && body == meth.BodyPtr()) {
    return methptr;
  }

  return make_shared<MethodDecl>(meth.Mods(), meth.GetTypePtr(), meth.Ident(), params, body);
}


REWRITE_DEFN2(Visitor2, ConstructorDecl, MemberDecl, meth) {
  SHORT_CIRCUIT(ConstructorDecl, meth);

  sptr<ParamList> params = meth.Params().Accept2(this, meth.ParamsPtr());
  sptr<Stmt> body = meth.Body().Accept2(this, meth.BodyPtr());

  if (params == nullptr || body == nullptr) {
    return nullptr;
  } else if (params == meth.ParamsPtr() && body == meth.BodyPtr()) {
    return methptr;
  }

  return make_shared<ConstructorDecl>(meth.Mods(), meth.Ident(), params, body);
}

REWRITE_DEFN2(Visitor2, ClassDecl, TypeDecl, type) {
  SHORT_CIRCUIT(ClassDecl, type);

  bool membersChanged = false;
  SharedPtrVector<MemberDecl> newMembers = AcceptMulti(type.Members(), &membersChanged);
  if (!membersChanged) {
    return typeptr;
  }

  return make_shared<ClassDecl>(type.Mods(), type.Name(), type.NameToken(), type.Interfaces(), newMembers, type.SuperPtr(), type.GetTypeId());
}

REWRITE_DEFN2(Visitor2, InterfaceDecl, TypeDecl, type) {
  SHORT_CIRCUIT(InterfaceDecl, type);

  bool membersChanged = false;
  SharedPtrVector<MemberDecl> newMembers = AcceptMulti(type.Members(), &membersChanged);
  if (!membersChanged) {
    return typeptr;
  }

  return make_shared<InterfaceDecl>(type.Mods(), type.Name(), type.NameToken(), type.Interfaces(), newMembers, type.GetTypeId());
}

REWRITE_DEFN2(Visitor2, CompUnit, CompUnit, unit) {
  SHORT_CIRCUIT(CompUnit, unit);

  bool typesChanged = false;
  SharedPtrVector<TypeDecl> newTypes = AcceptMulti(unit.Types(), &typesChanged);

  if (!typesChanged) {
    return unitptr;
  }
  return make_shared<CompUnit>(unit.PackagePtr(), unit.Imports(), newTypes);
}

REWRITE_DEFN2(Visitor2, Program, Program, prog) {
  SHORT_CIRCUIT(Program, prog);

  bool unitsChanged = false;
  SharedPtrVector<CompUnit> units = AcceptMulti(prog.CompUnits(), &unitsChanged);

  if (!unitsChanged) {
    return progptr;
  }

  return make_shared<Program>(units);
}
}  // namespace ast

