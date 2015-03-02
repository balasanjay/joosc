#include "ast/visitor.h"

#include "ast/ast.h"
#include "lexer/lexer.h"

namespace ast {

using lexer::Token;
using base::Error;
using base::FileSet;
using base::SharedPtrVector;

#define SHORT_CIRCUIT(type, var, varptr) \
  auto VISIT_RESULT = VisitResult::RECURSE; \
  {                                    \
    auto result = Visit##type(var, varptr); \
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
  SHORT_CIRCUIT(ArrayIndexExpr, expr, exprptr);

  sptr<const Expr> base = Rewrite(expr.BasePtr());
  sptr<const Expr> index = Rewrite(expr.IndexPtr());
  if (SHOULD_PRUNE_AFTER || base == nullptr || index == nullptr) {
    return nullptr;
  }
  if (base == expr.BasePtr() && index == expr.IndexPtr()) {
    return exprptr;
  }
  return make_shared<ArrayIndexExpr>(base, expr.Lbrack(), index, expr.Rbrack());
}

REWRITE_DEFN(Visitor, BinExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(BinExpr, expr, exprptr);

  sptr<const Expr> lhs = Rewrite(expr.LhsPtr());
  sptr<const Expr> rhs = Rewrite(expr.RhsPtr());
  if (SHOULD_PRUNE_AFTER || lhs == nullptr || rhs == nullptr) {
    return nullptr;
  }
  if (lhs == expr.LhsPtr() && rhs == expr.RhsPtr()) {
    return exprptr;
  }

  return make_shared<BinExpr>(lhs, expr.Op(), rhs);
}

REWRITE_DEFN(Visitor, CallExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CallExpr, expr, exprptr);

  bool argsChanged = false;
  sptr<const Expr> base = Rewrite(expr.BasePtr());
  SharedPtrVector<const Expr> args = AcceptMulti(expr.Args(), &argsChanged);

  if (SHOULD_PRUNE_AFTER || base == nullptr || args.Size() != expr.Args().Size()) {
    return nullptr;
  }
  if (base == expr.BasePtr() && !argsChanged) {
    return exprptr;
  }

  return make_shared<CallExpr>(base, expr.Lparen(), args, expr.Rparen());
}

REWRITE_DEFN(Visitor, CastExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CastExpr, expr, exprptr);

  sptr<const Expr> castedExpr = Rewrite(expr.GetExprPtr());
  if (SHOULD_PRUNE_AFTER || castedExpr == nullptr) {
    return nullptr;
  } else if (castedExpr == expr.GetExprPtr()) {
    return exprptr;
  }
  return make_shared<CastExpr>(expr.Lparen(), expr.GetTypePtr(), expr.Rparen(), castedExpr);
}

REWRITE_DEFN(Visitor, FieldDerefExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(FieldDerefExpr, expr, exprptr);
  sptr<const Expr> base = Rewrite(expr.BasePtr());
  if (SHOULD_PRUNE_AFTER || base == nullptr) {
    return nullptr;
  } else if (base == expr.BasePtr()) {
    return exprptr;
  }
  return make_shared<FieldDerefExpr>(base, expr.FieldName(), expr.GetToken());
}

REWRITE_DEFN(Visitor, BoolLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(BoolLitExpr, expr, exprptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, CharLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(CharLitExpr, expr, exprptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, StringLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(StringLitExpr, expr, exprptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}

REWRITE_DEFN(Visitor, StaticRefExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(StaticRefExpr, expr, exprptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}

REWRITE_DEFN(Visitor, NullLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NullLitExpr, expr, exprptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, IntLitExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(IntLitExpr, expr, exprptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}
REWRITE_DEFN(Visitor, NameExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NameExpr, expr, exprptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}

REWRITE_DEFN(Visitor, NewArrayExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NewArrayExpr, expr, exprptr);

  sptr<const Expr> arrayExpr = nullptr;
  if (expr.GetExprPtr() != nullptr) {
    arrayExpr = Rewrite(expr.GetExprPtr());
  }

  // We don't prune the subtree if the expr returns null, because the expr is a
  // nullable field.
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  } else if (arrayExpr == expr.GetExprPtr()) {
    return exprptr;
  }
  return make_shared<NewArrayExpr>(expr.NewToken(), expr.GetTypePtr(), expr.Lbrack(), arrayExpr, expr.Rbrack());
}

REWRITE_DEFN(Visitor, NewClassExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(NewClassExpr, expr, exprptr);

  bool argsChanged = false;
  SharedPtrVector<const Expr> args = AcceptMulti(expr.Args(), &argsChanged);

  if (SHOULD_PRUNE_AFTER || args.Size() != expr.Args().Size()) {
    return nullptr;
  }
  if (!argsChanged) {
    return exprptr;
  }
  return make_shared<NewClassExpr>(expr.NewToken(), expr.GetTypePtr(), expr.Lparen(), args, expr.Rparen());
}

REWRITE_DEFN(Visitor, ParenExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(ParenExpr, expr, exprptr);
  sptr<const Expr> nested = Rewrite(expr.NestedPtr());
  if (SHOULD_PRUNE_AFTER || nested == nullptr) {
    return nullptr;
  } else if (nested == expr.NestedPtr()) {
    return exprptr;
  }
  return make_shared<ParenExpr>(expr.Lparen(), nested, expr.Rparen());
}

REWRITE_DEFN(Visitor, ThisExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(ThisExpr, expr, exprptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return exprptr;
}

REWRITE_DEFN(Visitor, UnaryExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(UnaryExpr, expr, exprptr);
  sptr<const Expr> rhs = Rewrite(expr.RhsPtr());
  if (SHOULD_PRUNE_AFTER || rhs == nullptr) {
    return nullptr;
  } else if (rhs == expr.RhsPtr()) {
    return exprptr;
  }
  return make_shared<UnaryExpr>(expr.Op(), rhs);
}

REWRITE_DEFN(Visitor, InstanceOfExpr, Expr, expr, exprptr) {
  SHORT_CIRCUIT(InstanceOfExpr, expr, exprptr);
  sptr<const Expr> lhs = Rewrite(expr.LhsPtr());
  if (SHOULD_PRUNE_AFTER || lhs == nullptr) {
    return nullptr;
  } else if (lhs == expr.LhsPtr()) {
    return exprptr;
  }
  return make_shared<InstanceOfExpr>(lhs, expr.InstanceOf(), expr.GetTypePtr());
}

REWRITE_DEFN(Visitor, BlockStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(BlockStmt, stmt, stmtptr);

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
  SHORT_CIRCUIT(EmptyStmt, stmt, stmtptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return stmtptr;
}

REWRITE_DEFN(Visitor, ExprStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(ExprStmt, stmt, stmtptr);

  sptr<const Expr> expr = Rewrite(stmt.GetExprPtr());
  if (SHOULD_PRUNE_AFTER || expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<ExprStmt>(expr);
}

REWRITE_DEFN(Visitor, LocalDeclStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(LocalDeclStmt, stmt, stmtptr);

  sptr<const Expr> expr = Rewrite(stmt.GetExprPtr());
  if (SHOULD_PRUNE_AFTER || expr == nullptr) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<LocalDeclStmt>(stmt.GetTypePtr(), stmt.Name(), stmt.NameToken(), expr);
}

REWRITE_DEFN(Visitor, ReturnStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(ReturnStmt, stmt, stmtptr);

  sptr<const Expr> expr = nullptr;
  if (stmt.GetExprPtr() != nullptr) {
    expr = Rewrite(stmt.GetExprPtr());
  }

  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  } else if (expr == stmt.GetExprPtr()) {
    return stmtptr;
  }

  return make_shared<ReturnStmt>(stmt.ReturnToken(), expr);
}

REWRITE_DEFN(Visitor, IfStmt, Stmt, stmt, stmtptr) {
  SHORT_CIRCUIT(IfStmt, stmt, stmtptr);

  sptr<const Expr> cond = Rewrite(stmt.CondPtr());
  if (cond == nullptr) {
    return nullptr;
  }

  sptr<const Stmt> trueBody = Rewrite(stmt.TrueBodyPtr());
  sptr<const Stmt> falseBody = Rewrite(stmt.FalseBodyPtr());

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
  SHORT_CIRCUIT(ForStmt, stmt, stmtptr);

  sptr<const Stmt> init = Rewrite(stmt.InitPtr());

  sptr<const Expr> cond = nullptr;
  if (stmt.CondPtr() != nullptr) {
    cond = Rewrite(stmt.CondPtr());
  }

  sptr<const Expr> update = nullptr;
  if (stmt.UpdatePtr() != nullptr) {
    update = Rewrite(stmt.UpdatePtr());
  }

  sptr<const Stmt> body = Rewrite(stmt.BodyPtr());
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
  SHORT_CIRCUIT(WhileStmt, stmt, stmtptr);

  sptr<const Expr> cond = Rewrite(stmt.CondPtr());
  sptr<const Stmt> body = Rewrite(stmt.BodyPtr());

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
  SHORT_CIRCUIT(ParamList, params, paramsptr);

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
  SHORT_CIRCUIT(Param, param, paramptr);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  return paramptr;
}

REWRITE_DEFN(Visitor, FieldDecl, MemberDecl, field, fieldptr) {
  SHORT_CIRCUIT(FieldDecl, field, fieldptr);

  sptr<const Expr> val;
  if (field.ValPtr() != nullptr) {
    val = Rewrite(field.ValPtr());
  }

  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  if (val == field.ValPtr()) {
    return fieldptr;
  }

  return make_shared<FieldDecl>(field.Mods(), field.GetTypePtr(), field.Name(), field.NameToken(), val);
}

REWRITE_DEFN(Visitor, MethodDecl, MemberDecl, meth, methptr) {
  SHORT_CIRCUIT(MethodDecl, meth, methptr);

  sptr<const ParamList> params = Rewrite(meth.ParamsPtr());
  sptr<const Stmt> body = Rewrite(meth.BodyPtr());

  if (SHOULD_PRUNE_AFTER || params == nullptr || body == nullptr) {
    return nullptr;
  } else if (params == meth.ParamsPtr() && body == meth.BodyPtr()) {
    return methptr;
  }

  return make_shared<MethodDecl>(meth.Mods(), meth.TypePtr(), meth.Name(), meth.NameToken(), params, body);
}

REWRITE_DEFN(Visitor, TypeDecl, TypeDecl, type, typeptr) {
  SHORT_CIRCUIT(TypeDecl, type, typeptr);

  bool membersChanged = false;
  SharedPtrVector<const MemberDecl> newMembers = AcceptMulti(type.Members(), &membersChanged);
  if (SHOULD_PRUNE_AFTER) {
    return nullptr;
  }
  if (!membersChanged) {
    return typeptr;
  }

  return make_shared<TypeDecl>(type.Mods(), type.Kind(), type.Name(), type.NameToken(), type.Extends(), type.Implements(), newMembers, type.GetTypeId());
}

REWRITE_DEFN(Visitor, CompUnit, CompUnit, unit, unitptr) {
  SHORT_CIRCUIT(CompUnit, unit, unitptr);

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
  auto result = VisitProgram(prog, progptr);
  if (result == VisitResult::SKIP) {
    return progptr;
  }
  CHECK(result == VisitResult::RECURSE);

  bool unitsChanged = false;
  SharedPtrVector<const CompUnit> units = AcceptMulti(prog.CompUnits(), &unitsChanged);

  if (!unitsChanged) {
    return progptr;
  }

  return make_shared<Program>(units);
}
}  // namespace ast

