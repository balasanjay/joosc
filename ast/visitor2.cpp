#include "ast/visitor2.h"

#include "ast/ast.h"
#include "lexer/lexer.h"

namespace ast {

using lexer::Token;
using base::Error;
using base::FileSet;
using base::SharedPtrVector;

#define SHORT_CIRCUIT(type, var) \
  auto VISIT_RESULT = VisitResult::RECURSE; \
  {                                    \
    auto result = Visit##type(var); \
    if (result == VisitResult::SKIP) { \
      return var##ptr; \
    } \
    if (result == VisitResult::SKIP_PRUNE) { \
      return nullptr; \
    } \
    VISIT_RESULT = result; \
  }

#define SHOULD_PRUNE_AFTER (VISIT_RESULT == VisitResult::RECURSE_PRUNE)


REWRITE_DEFN(Visitor, ArrayIndexExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(ArrayIndexExpr, expr);

  sptr<const Expr> base = Visit(this, expr.BasePtr());
  sptr<const Expr> index = Visit(this, expr.IndexPtr());
  if (SHOULD_PRUNE_AFTER || base == nullptr || index == nullptr) {
    return nullptr;
  }
  if (base == expr.BasePtr() && index == expr.IndexPtr()) {
    return exprptr;
  }
  return make_shared<ArrayIndexExpr>(base, index);
}

REWRITE_DEFN(Visitor, BinExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(BinExpr, expr);

  sptr<const Expr> lhs = Visit(this, expr.LhsPtr());
  sptr<const Expr> rhs = Visit(this, expr.RhsPtr());
  if (SHOULD_PRUNE_AFTER || lhs == nullptr || rhs == nullptr) {
    return nullptr;
  }
  if (lhs == expr.LhsPtr() && rhs == expr.RhsPtr()) {
    return exprptr;
  }

  return make_shared<BinExpr>(lhs, expr.Op(), rhs);
}

REWRITE_DEFN(Visitor, CallExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CallExpr, expr);

  bool argsChanged = false;
  sptr<const Expr> base = Visit(this, expr.BasePtr());
  SharedPtrVector<const Expr> args = AcceptMulti(expr.Args(), &argsChanged);

  if (SHOULD_PRUNE_AFTER || base == nullptr || args.Size() != expr.Args().Size()) {
    return nullptr;
  }
  if (base == expr.BasePtr() && !argsChanged) {
    return exprptr;
  }

  return make_shared<CallExpr>(base, expr.Lparen(), args);
}

REWRITE_DEFN(Visitor, CastExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CastExpr, expr);

  sptr<const Expr> castedExpr = Visit(this, expr.GetExprPtr());
  if (SHOULD_PRUNE_AFTER || castedExpr == nullptr) {
    return nullptr;
  } else if (castedExpr == expr.GetExprPtr()) {
    return exprptr;
  }
  return make_shared<CastExpr>(expr.GetTypePtr(), castedExpr);
}

REWRITE_DEFN(Visitor, FieldDerefExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(FieldDerefExpr, expr);
  sptr<const Expr> base = Visit(this, expr.BasePtr());
  if (SHOULD_PRUNE_AFTER || base == nullptr) {
    return nullptr;
  } else if (base == expr.BasePtr()) {
    return exprptr;
  }
  return make_shared<FieldDerefExpr>(base, expr.FieldName(), expr.GetToken());
}

REWRITE_DEFN(Visitor, BoolLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(BoolLitExpr, expr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, CharLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CharLitExpr, expr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, StringLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(StringLitExpr, expr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, NullLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NullLitExpr, expr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, IntLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(IntLitExpr, expr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, NameExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NameExpr, expr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}

REWRITE_DEFN(Visitor, NewArrayExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NewArrayExpr, expr);

  sptr<const Expr> arrayExpr = nullptr;
  if (expr.GetExprPtr() != nullptr) {
    arrayExpr = Visit(this, expr.GetExprPtr());
  }

  // We don't prune the subtree if the expr returns null, because the expr is a
  // nullable field.
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  } else if (arrayExpr == expr.GetExprPtr()) {
    return exprptr;
  }
  return make_shared<NewArrayExpr>(expr.GetTypePtr(), arrayExpr);
}

REWRITE_DEFN(Visitor, NewClassExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NewClassExpr, expr);

  bool argsChanged = false;
  SharedPtrVector<const Expr> args = AcceptMulti(expr.Args(), &argsChanged);

  if (SHOULD_PRUNE_AFTER || args.Size() != expr.Args().Size()) {
    return nullptr;
  }
  if (!argsChanged) {
    return exprptr;
  }
  return make_shared<NewClassExpr>(expr.NewToken(), expr.GetTypePtr(), args);
}

REWRITE_DEFN(Visitor, ParenExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(ParenExpr, expr);
  sptr<const Expr> nested = Visit(this, expr.NestedPtr());
  if (SHOULD_PRUNE_AFTER || nested == nullptr) {
    return nullptr;
  } else if (nested == expr.NestedPtr()) {
    return exprptr;
  }
  return make_shared<ParenExpr>(nested);
}

REWRITE_DEFN(Visitor, ThisExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(ThisExpr, expr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}

REWRITE_DEFN(Visitor, UnaryExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(UnaryExpr, expr);
  sptr<const Expr> rhs = Visit(this, expr.RhsPtr());
  if (SHOULD_PRUNE_AFTER || rhs == nullptr) {
    return nullptr;
  } else if (rhs == expr.RhsPtr()) {
    return exprptr;
  }
  return make_shared<UnaryExpr>(expr.Op(), rhs);
}

REWRITE_DEFN(Visitor, InstanceOfExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(InstanceOfExpr, expr);
  sptr<const Expr> lhs = Visit(this, expr.LhsPtr());
  if (SHOULD_PRUNE_AFTER || lhs == nullptr) {
    return nullptr;
  } else if (lhs == expr.LhsPtr()) {
    return exprptr;
  }
  return make_shared<InstanceOfExpr>(lhs, expr.InstanceOf(), expr.GetTypePtr());
}

REWRITE_DEFN(Visitor, BlockStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(BlockStmt, stmt);

  bool stmtsChanged = false;
  SharedPtrVector<const Stmt> newStmts = AcceptMulti(stmt.Stmts(), &stmtsChanged);

  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  if (!stmtsChanged) {
    return stmtptr;
  }

  return make_shared<BlockStmt>(newStmts);
}

REWRITE_DEFN(Visitor, EmptyStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(EmptyStmt, stmt);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return stmtptr;
}

REWRITE_DEFN(Visitor, ExprStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(ExprStmt, stmt);

  sptr<const Expr> expr = Visit(this, stmt.GetExprPtr());
  if (SHOULD_PRUNE_AFTER || expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<ExprStmt>(expr);
}

REWRITE_DEFN(Visitor, LocalDeclStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(LocalDeclStmt, stmt);

  sptr<const Expr> expr = Visit(this, stmt.GetExprPtr());
  if (SHOULD_PRUNE_AFTER || expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<LocalDeclStmt>(stmt.GetTypePtr(), stmt.Ident(), expr);
}

REWRITE_DEFN(Visitor, ReturnStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(ReturnStmt, stmt);

  sptr<const Expr> expr = nullptr;
  if (stmt.GetExprPtr() != nullptr) {
    expr = Visit(this, stmt.GetExprPtr());
  }

  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<ReturnStmt>(expr);
}

REWRITE_DEFN(Visitor, IfStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(IfStmt, stmt);

  sptr<const Expr> cond = Visit(this, stmt.CondPtr());
  if (cond == nullptr) {
    return nullptr;
  }

  sptr<const Stmt> trueBody = Visit(this, stmt.TrueBodyPtr());
  sptr<const Stmt> falseBody = Visit(this, stmt.FalseBodyPtr());

  // If a subtree was pruned, then make it an empty statement.
  if (trueBody == nullptr) {
    trueBody = make_shared<EmptyStmt>();
  }
  if (falseBody == nullptr) {
    falseBody = make_shared<EmptyStmt>();
  }

  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  if (cond == stmt.CondPtr() && trueBody == stmt.TrueBodyPtr() && falseBody == stmt.FalseBodyPtr()) {
    return stmtptr;
  }

  return make_shared<IfStmt>(cond, trueBody, falseBody);
}

REWRITE_DEFN(Visitor, ForStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(ForStmt, stmt);

  sptr<const Stmt> init = Visit(this, stmt.InitPtr());

  sptr<const Expr> cond = nullptr;
  if (stmt.CondPtr() != nullptr) {
    cond = Visit(this, stmt.CondPtr());
  }

  sptr<const Expr> update = nullptr;
  if (stmt.UpdatePtr() != nullptr) {
    update = Visit(this, stmt.UpdatePtr());
  }

  sptr<const Stmt> body = Visit(this, stmt.BodyPtr());
  if (body == nullptr) {
    body = make_shared<EmptyStmt>();
  }

  if (SHOULD_PRUNE_AFTER || init == nullptr) {
    return nullptr;
  } else if (init == stmt.InitPtr() && cond == stmt.CondPtr() && update == stmt.UpdatePtr() && body == stmt.BodyPtr()) {
    return stmtptr;
  }

  return make_shared<ForStmt>(init, cond, update, body);
}

REWRITE_DEFN(Visitor, WhileStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(WhileStmt, stmt);

  sptr<const Expr> cond = Visit(this, stmt.CondPtr());
  sptr<const Stmt> body = Visit(this, stmt.BodyPtr());

  if (SHOULD_PRUNE_AFTER || cond == nullptr) {
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

REWRITE_DEFN(Visitor, ParamList, ParamList, params, paramsptr) {
  SHORT_CIRCUIT(ParamList, params);

  bool paramsChanged = false;
  SharedPtrVector<const Param> newParams = AcceptMulti(params.Params(), &paramsChanged);

  if (SHOULD_PRUNE_AFTER || newParams.Size() != params.Params().Size()) {
    return nullptr;
  }
  if (!paramsChanged) {
    return paramsptr;
  }

  return make_shared<ParamList>(newParams);
}


REWRITE_DEFN(Visitor, Param, Param, param, paramptr) {
  SHORT_CIRCUIT(Param, param);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return paramptr;
}

REWRITE_DEFN(Visitor, FieldDecl, MemberDecl, field, fieldptr) {
  SHORT_CIRCUIT(FieldDecl, field);

  sptr<const Expr> val;
  if (field.ValPtr() != nullptr) {
    val = Visit(this, field.ValPtr());
  }

  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  if (val == field.ValPtr()) {
    return fieldptr;
  }

  return make_shared<FieldDecl>(field.Mods(), field.GetTypePtr(), field.Ident(), val);
}

REWRITE_DEFN(Visitor, MethodDecl, MemberDecl, meth, methptr) {
  SHORT_CIRCUIT(MethodDecl, meth);

  sptr<const ParamList> params = Visit(this, meth.ParamsPtr());
  sptr<const Stmt> body = Visit(this, meth.BodyPtr());

  if (SHOULD_PRUNE_AFTER || params == nullptr || body == nullptr) {
    return nullptr;
  } else if (params == meth.ParamsPtr() && body == meth.BodyPtr()) {
    return methptr;
  }

  return make_shared<MethodDecl>(meth.Mods(), meth.GetTypePtr(), meth.Ident(), params, body);
}


REWRITE_DEFN(Visitor, ConstructorDecl, MemberDecl, meth, methptr) {
  SHORT_CIRCUIT(ConstructorDecl, meth);

  sptr<const ParamList> params = Visit(this, meth.ParamsPtr());
  sptr<const Stmt> body = Visit(this, meth.BodyPtr());

  if (SHOULD_PRUNE_AFTER || params == nullptr || body == nullptr) {
    return nullptr;
  } else if (params == meth.ParamsPtr() && body == meth.BodyPtr()) {
    return methptr;
  }

  return make_shared<ConstructorDecl>(meth.Mods(), meth.Ident(), params, body);
}

REWRITE_DEFN(Visitor, ClassDecl, TypeDecl, type, typeptr) {
  SHORT_CIRCUIT(ClassDecl, type);

  bool membersChanged = false;
  SharedPtrVector<const MemberDecl> newMembers = AcceptMulti(type.Members(), &membersChanged);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  if (!membersChanged) {
    return typeptr;
  }

  return make_shared<ClassDecl>(type.Mods(), type.Name(), type.NameToken(), type.Interfaces(), newMembers, type.SuperPtr(), type.GetTypeId());
}

REWRITE_DEFN(Visitor, InterfaceDecl, TypeDecl, type, typeptr) {
  SHORT_CIRCUIT(InterfaceDecl, type);

  bool membersChanged = false;
  SharedPtrVector<const MemberDecl> newMembers = AcceptMulti(type.Members(), &membersChanged);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  if (!membersChanged) {
    return typeptr;
  }

  return make_shared<InterfaceDecl>(type.Mods(), type.Name(), type.NameToken(), type.Interfaces(), newMembers, type.GetTypeId());
}

REWRITE_DEFN(Visitor, CompUnit, CompUnit, unit, unitptr) {
  SHORT_CIRCUIT(CompUnit, unit);

  bool typesChanged = false;
  SharedPtrVector<const TypeDecl> newTypes = AcceptMulti(unit.Types(), &typesChanged);

  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  if (!typesChanged) {
    return unitptr;
  }
  return make_shared<CompUnit>(unit.PackagePtr(), unit.Imports(), newTypes);
}

REWRITE_DEFN(Visitor, Program, Program, prog, progptr) {
  // We special-case the short-circuiting for program, because the other two
  // would result in nullable programs.
  auto result = VisitProgram(prog);
  if (result == VisitResult::SKIP) {
    return progptr;
  }
  assert(result == VisitResult::RECURSE);

  bool unitsChanged = false;
  SharedPtrVector<const CompUnit> units = AcceptMulti(prog.CompUnits(), &unitsChanged);

  if (!unitsChanged) {
    return progptr;
  }

  return make_shared<Program>(units);
}
}  // namespace ast

