#include "types/typechecker.h"

#include "ast/ast.h"
#include "ast/extent.h"
#include "base/error.h"
#include "base/macros.h"
#include "types/types_internal.h"

using namespace ast;

using base::Error;
using base::FileSet;
using base::Pos;
using base::PosRange;
using base::SharedPtrVector;
using lexer::TokenType;

namespace types {

REWRITE_DEFN(TypeChecker, ArrayIndexExpr, Expr, expr,) {
  sptr<const Expr> base = Rewrite(expr.BasePtr());
  sptr<const Expr> index = Rewrite(expr.IndexPtr());
  if (base == nullptr || index == nullptr) {
    return nullptr;
  }

  if (!IsNumeric(index->GetTypeId())) {
    errors_->Append(MakeTypeMismatchError(TypeId{TypeId::kIntBase, 0}, index->GetTypeId(), ExtentOf(index)));
    return nullptr;
  }
  if (base->GetTypeId().ndims < 1) {
    errors_->Append(MakeIndexNonArrayError(ExtentOf(base)));
    return nullptr;
  }

  TypeId tid = TypeId{base->GetTypeId().base, base->GetTypeId().ndims - 1};
  return make_shared<ArrayIndexExpr>(base, expr.Lbrack(), index, expr.Rbrack(), tid);
}

bool IsBoolOp(TokenType op) {
  switch (op) {
    case lexer::BAND:
    case lexer::BOR:
    case lexer::AND:
    case lexer::OR:
    case lexer::XOR:
      return true;
    default:
      return false;
  }
}

bool IsRelationalOp(TokenType op) {
  switch (op) {
    case lexer::LE:
    case lexer::GE:
    case lexer::LT:
    case lexer::GT:
      return true;
    default:
      return false;
  }
}

bool IsEqualityOp(TokenType op) {
  switch (op) {
    case lexer::EQ:
    case lexer::NEQ:
      return true;
    default:
      return false;
  }
}

bool IsNumericOp(TokenType op) {
  switch (op) {
    case lexer::ADD:
    case lexer::SUB:
    case lexer::MUL:
    case lexer::DIV:
    case lexer::MOD:
      return true;
    default:
      return false;
  }
}

REWRITE_DEFN(TypeChecker, BinExpr, Expr, expr, ) {
  const TypeId kBoolTypeId = TypeId{TypeId::kBoolBase, 0};
  const TypeId kIntTypeId = TypeId{TypeId::kIntBase, 0};

  sptr<const Expr> lhs = Rewrite(expr.LhsPtr());
  sptr<const Expr> rhs = Rewrite(expr.RhsPtr());
  if (lhs == nullptr || rhs == nullptr) {
    return nullptr;
  }

  TypeId lhsType = lhs->GetTypeId();
  TypeId rhsType = rhs->GetTypeId();
  TokenType op = expr.Op().type;

  if (op == lexer::ASSG) {
    // TODO: implement assignment.
    return nullptr;
  }

  if (IsBoolOp(op)) {
    if (lhsType == kBoolTypeId && rhsType == kBoolTypeId) {
      return make_shared<BinExpr>(lhs, expr.Op(), rhs, kBoolTypeId);
    }

    if (lhsType != kBoolTypeId) {
      errors_->Append(MakeTypeMismatchError(kBoolTypeId, lhsType, ExtentOf(expr.LhsPtr())));
    }
    if (rhsType != kBoolTypeId) {
      errors_->Append(MakeTypeMismatchError(kBoolTypeId, rhsType, ExtentOf(expr.RhsPtr())));
    }
    return nullptr;
  }

  if (IsRelationalOp(op)) {
    if (IsNumeric(lhsType) && IsNumeric(rhsType)) {
      return make_shared<BinExpr>(lhs, expr.Op(), rhs, kBoolTypeId);
    }

    if (!IsNumeric(lhsType)) {
      errors_->Append(MakeTypeMismatchError(kIntTypeId, lhsType, ExtentOf(expr.LhsPtr())));
    }
    if (!IsNumeric(rhsType)) {
      errors_->Append(MakeTypeMismatchError(kIntTypeId, rhsType, ExtentOf(expr.RhsPtr())));
    }
    return nullptr;
  }

  if (IsEqualityOp(op)) {
    if (!IsComparable(lhsType, rhsType)) {
      errors_->Append(MakeIncomparableTypeError(lhsType, rhsType, expr.Op().pos));
      return nullptr;
    }
    return make_shared<BinExpr>(lhs, expr.Op(), rhs, kBoolTypeId);
  }

  const TypeId kStrType = typeset_.Get({"java", "lang", "String"});
  if (op == lexer::ADD && !kStrType.IsError() && (lhsType == kStrType || rhsType == kStrType)) {
    return make_shared<BinExpr>(lhs, expr.Op(), rhs, kStrType);
  }

  assert(IsNumericOp(op));
  if (IsNumeric(lhsType) && IsNumeric(rhsType)) {
    return make_shared<BinExpr>(lhs, expr.Op(), rhs, kIntTypeId);
  }

  if (!IsNumeric(lhsType)) {
    errors_->Append(MakeTypeMismatchError(kIntTypeId, lhsType, ExtentOf(expr.LhsPtr())));
  }
  if (!IsNumeric(rhsType)) {
    errors_->Append(MakeTypeMismatchError(kIntTypeId, rhsType, ExtentOf(expr.RhsPtr())));
  }
  return nullptr;
}

REWRITE_DEFN(TypeChecker, BoolLitExpr, Expr, expr, ) {
  return make_shared<BoolLitExpr>(expr.GetToken(), TypeId{TypeId::kBoolBase, 0});
}

// TODO: CallExpr

REWRITE_DEFN(TypeChecker, CastExpr, Expr, expr, exprptr) {
  sptr<const Expr> castedExpr = Rewrite(expr.GetExprPtr());
  if (castedExpr == nullptr) {
    return nullptr;
  }
  TypeId exprType = castedExpr->GetTypeId();

  TypeId castType = MustResolveType(expr.GetType());
  if (castType.IsError()) {
    return nullptr;
  }

  if ((IsPrimitive(castType) && IsReference(exprType)) ||
      (IsReference(castType) && IsPrimitive(exprType))) {
    errors_->Append(MakeIncompatibleCastError(castType, exprType, ExtentOf(exprptr)));
    return nullptr;
  }

  // If either way is assignable, then this is allowed. Simplification for Joos.
  if (!IsAssignable(castType, exprType) && !IsAssignable(exprType, castType)) {
    errors_->Append(MakeIncompatibleCastError(castType, exprType, ExtentOf(exprptr)));
    return nullptr;
  }

  return make_shared<CastExpr>(expr.Lparen(), expr.GetTypePtr(), expr.Rparen(), castedExpr, castType);
}

REWRITE_DEFN(TypeChecker, CharLitExpr, Expr, expr, ) {
  return make_shared<CharLitExpr>(expr.GetToken(), TypeId{TypeId::kCharBase, 0});
}

// TODO: FieldDerefExpr

REWRITE_DEFN(TypeChecker, InstanceOfExpr, Expr, expr, exprptr) {
  sptr<const Expr> lhs = Rewrite(expr.LhsPtr());
  if (lhs == nullptr) {
    return nullptr;
  }
  TypeId lhsType = lhs->GetTypeId();

  TypeId instanceOfType = MustResolveType(expr.GetType());
  if (instanceOfType.IsError()) {
    return nullptr;
  }

  if (IsPrimitive(lhsType) || IsPrimitive(instanceOfType)) {
    errors_->Append(MakeInstanceOfPrimitiveError(ExtentOf(exprptr)));
    return nullptr;
  }

  // If either way is assignable, then this is allowed. Simplification for Joos.
  if (!IsAssignable(lhsType, instanceOfType) && !IsAssignable(instanceOfType, lhsType)) {
    errors_->Append(MakeIncompatibleInstanceOfError(lhsType, instanceOfType, ExtentOf(exprptr)));
    return nullptr;
  }

  return make_shared<InstanceOfExpr>(lhs, expr.InstanceOf(), expr.GetTypePtr(), TypeId{TypeId::kBoolBase, 0});
}

REWRITE_DEFN(TypeChecker, IntLitExpr, Expr, expr, ) {
  return make_shared<IntLitExpr>(expr.GetToken(), expr.Value(), TypeId{TypeId::kIntBase, 0});
}

// TODO: NameExpr

REWRITE_DEFN(TypeChecker, NewArrayExpr, Expr, expr,) {
  TypeId tid = MustResolveType(expr.GetType());
  if (tid.IsError()) {
    return nullptr;
  }

  sptr<const Expr> index;
  TypeId expectedIndexType = TypeId{TypeId::kIntBase, 0};

  if (expr.GetExprPtr() != nullptr) {
    index = Rewrite(expr.GetExprPtr());
  }
  if (index != nullptr && index->GetTypeId() != expectedIndexType) {
    errors_->Append(MakeTypeMismatchError(expectedIndexType, index->GetTypeId(), ExtentOf(index)));
    return nullptr;
  }

  return make_shared<NewArrayExpr>(expr.NewToken(), expr.GetTypePtr(), expr.Lbrack(), index, expr.Rbrack(), TypeId{tid.base, tid.ndims + 1});
}

// TODO: NewClassExpr

REWRITE_DEFN(TypeChecker, NullLitExpr, Expr, expr, ) {
  return make_shared<NullLitExpr>(expr.GetToken(), TypeId{TypeId::kNullBase, 0});
}

REWRITE_DEFN(TypeChecker, ParenExpr, Expr, expr,) {
  return Rewrite(expr.NestedPtr());
}

REWRITE_DEFN(TypeChecker, StringLitExpr, Expr, expr,) {
  TypeId strType = typeset_.Get({"java", "lang", "String"});
  if (strType.IsError()) {
    errors_->Append(MakeNoStringError(expr.GetToken().pos));
    return nullptr;
  }

  return make_shared<StringLitExpr>(expr.GetToken(), strType);
}

REWRITE_DEFN(TypeChecker, ThisExpr, Expr, expr,) {
  return make_shared<ThisExpr>(expr.ThisToken(), curtype_);
}

REWRITE_DEFN(TypeChecker, UnaryExpr, Expr, expr, exprptr) {
  sptr<const Expr> rhs = Rewrite(expr.RhsPtr());
  if (rhs == nullptr) {
    return nullptr;
  }
  TypeId rhsType = rhs->GetTypeId();

  TokenType op = expr.Op().type;

  if (op == lexer::SUB) {
    if (!IsNumeric(rhsType)) {
      errors_->Append(MakeUnaryNonNumericError(rhsType, ExtentOf(exprptr)));
      return nullptr;
    }
    return make_shared<UnaryExpr>(expr.Op(), rhs, TypeId{TypeId::kIntBase, 0});
  }

  assert(op == lexer::NOT);
  TypeId expected = TypeId{TypeId::kBoolBase, 0};
  if (rhsType != expected) {
    errors_->Append(MakeUnaryNonBoolError(rhsType, ExtentOf(exprptr)));
    return nullptr;
  }
  return make_shared<UnaryExpr>(expr.Op(), rhs, TypeId{TypeId::kBoolBase, 0});
}

REWRITE_DEFN(TypeChecker, ForStmt, Stmt, stmt,) {
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

  if (init == nullptr || body == nullptr) {
    return nullptr;
  }

  TypeId expected = TypeId{TypeId::kBoolBase, 0};
  if (cond != nullptr && cond->GetTypeId() != expected) {
    errors_->Append(MakeTypeMismatchError(expected, cond->GetTypeId(), ExtentOf(stmt.CondPtr())));
    return nullptr;
  }

  return make_shared<ForStmt>(init, cond, update, body);
}

REWRITE_DEFN(TypeChecker, IfStmt, Stmt, stmt,) {
  sptr<const Expr> cond = Rewrite(stmt.CondPtr());
  sptr<const Stmt> trueBody = Rewrite(stmt.TrueBodyPtr());
  sptr<const Stmt> falseBody = Rewrite(stmt.FalseBodyPtr());

  if (cond == nullptr || trueBody == nullptr || falseBody == nullptr) {
    return nullptr;
  }

  TypeId expected = TypeId{TypeId::kBoolBase, 0};
  if (cond->GetTypeId() != expected) {
    errors_->Append(MakeTypeMismatchError(expected, cond->GetTypeId(), ExtentOf(stmt.CondPtr())));
    return nullptr;
  }

  return make_shared<IfStmt>(cond, trueBody, falseBody);
}

REWRITE_DEFN(TypeChecker, LocalDeclStmt, Stmt, stmt,) {
  sptr<const Expr> expr = Rewrite(stmt.GetExprPtr());
  TypeId lhsType = MustResolveType(stmt.GetType());

  if (lhsType.IsError() || expr == nullptr) {
    return nullptr;
  }

  if (!IsAssignable(lhsType, expr->GetTypeId())) {
    errors_->Append(MakeUnassignableError(lhsType, expr->GetTypeId(), ExtentOf(expr)));
    return nullptr;
  }

  // TODO: put into symbol table, and assign local variable id.

  return make_shared<LocalDeclStmt>(stmt.GetTypePtr(), stmt.Name(), stmt.NameToken(), expr);
}

REWRITE_DEFN(TypeChecker, ReturnStmt, Stmt, stmt,) {
  sptr<const Expr> expr = nullptr;
  if (stmt.GetExprPtr() != nullptr) {
    expr = Rewrite(stmt.GetExprPtr());
    if (expr == nullptr) {
      return nullptr;
    }
  }

  TypeId exprType = TypeId{TypeId::kVoidBase, 0};
  if (expr != nullptr) {
    exprType = expr->GetTypeId();
  }

  assert(belowMethodDecl_);
  if (!IsAssignable(curMethRet_, exprType)) {
    errors_->Append(MakeInvalidReturnError(curMethRet_, exprType, stmt.ReturnToken().pos));
    return nullptr;
  }

  return make_shared<ReturnStmt>(stmt.ReturnToken(), expr);
}


REWRITE_DEFN(TypeChecker, WhileStmt, Stmt, stmt,) {
  sptr<const Expr> cond = Rewrite(stmt.CondPtr());
  sptr<const Stmt> body = Rewrite(stmt.BodyPtr());

  if (cond == nullptr || body == nullptr) {
    return nullptr;
  }

  TypeId expected = TypeId{TypeId::kBoolBase, 0};
  if (cond->GetTypeId() != expected) {
    errors_->Append(MakeTypeMismatchError(expected, cond->GetTypeId(), ExtentOf(stmt.CondPtr())));
    return nullptr;
  }

  return make_shared<WhileStmt>(cond, body);
}

REWRITE_DEFN(TypeChecker, FieldDecl, MemberDecl, decl,) {
  TypeId lhsType = MustResolveType(decl.GetType());
  if (lhsType.IsError()) {
    return nullptr;
  }

  sptr<const Expr> val;
  if (decl.ValPtr() != nullptr) {
    val = Rewrite(decl.ValPtr());
    if (val == nullptr) {
      return nullptr;
    }

    if (!IsAssignable(lhsType, val->GetTypeId())) {
      errors_->Append(MakeUnassignableError(lhsType, val->GetTypeId(), ExtentOf(decl.ValPtr())));
      return nullptr;
    }
  }

  // TODO: When we start putting field-ids into FieldDecl, then this should
  // also populate it.

  return make_shared<FieldDecl>(decl.Mods(), decl.GetTypePtr(), decl.Name(), decl.NameToken(), val);
}

REWRITE_DEFN(TypeChecker, MethodDecl, MemberDecl, decl, declptr) {
  // If we have method info, then just use the default implementation of
  // RewriteMethodDecl.
  if (belowMethodDecl_) {
    return Visitor::RewriteMethodDecl(decl, declptr);
  }

  // Otherwise create a sub-visitor that has the method info, and let it
  // rewrite this node.

  TypeId rettype = TypeId{TypeId::kVoidBase, 0};
  if (decl.TypePtr() != nullptr) {
    rettype = MustResolveType(*decl.TypePtr());

    // This should have been pruned by previous pass if the type is invalid.
    assert(!rettype.IsError());
  }

  TypeChecker below(typeinfo_, typeset_, fs_, errors_, true, package_, true, curtype_, true, rettype);
  return below.Rewrite(declptr);
}

REWRITE_DEFN(TypeChecker, TypeDecl, TypeDecl, type, typeptr) {
  // If we have method info, then just use the default implementation of
  // RewriteTypeDecl.
  if (belowTypeDecl_) {
    return Visitor::RewriteTypeDecl(type, typeptr);
  }

  // Otherwise create a sub-visitor that has the type info, and let it rewrite
  // this node.

  vector<string> classname;
  if (package_ != nullptr) {
    classname = package_->Parts();
  }
  classname.push_back(type.Name());

  TypeId curtid = typeset_.Get(classname);
  assert(!curtid.IsError()); // Pruned in DeclResolver.

  TypeChecker below(typeinfo_, typeset_, fs_, errors_, true, package_, true, curtid);
  return below.Rewrite(typeptr);
}

REWRITE_DEFN(TypeChecker, CompUnit, CompUnit, unit, unitptr) {
  // If we have import info, then just use the default implementation of
  // RewriteCompUnit.
  if (belowCompUnit_) {
    return Visitor::RewriteCompUnit(unit, unitptr);
  }

  // Otherwise create a sub-visitor that has the import info, and let it
  // rewrite this node.

  TypeSet scopedTypeSet = typeset_.WithImports(unit.Imports(), fs_, errors_);
  TypeChecker below(typeinfo_, scopedTypeSet, fs_, errors_, true, unit.PackagePtr());

  return below.Rewrite(unitptr);
}

} // namespace types
