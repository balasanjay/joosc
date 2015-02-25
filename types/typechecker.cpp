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
    errors_->Append(MakeTypeMismatchError(TypeId::kInt, index->GetTypeId(), ExtentOf(index)));
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
    if (lhsType == TypeId::kBool && rhsType == TypeId::kBool) {
      return make_shared<BinExpr>(lhs, expr.Op(), rhs, TypeId::kBool);
    }

    if (lhsType != TypeId::kBool) {
      errors_->Append(MakeTypeMismatchError(TypeId::kBool, lhsType, ExtentOf(expr.LhsPtr())));
    }
    if (rhsType != TypeId::kBool) {
      errors_->Append(MakeTypeMismatchError(TypeId::kBool, rhsType, ExtentOf(expr.RhsPtr())));
    }
    return nullptr;
  }

  if (IsRelationalOp(op)) {
    if (IsNumeric(lhsType) && IsNumeric(rhsType)) {
      return make_shared<BinExpr>(lhs, expr.Op(), rhs, TypeId::kBool);
    }

    if (!IsNumeric(lhsType)) {
      errors_->Append(MakeTypeMismatchError(TypeId::kInt, lhsType, ExtentOf(expr.LhsPtr())));
    }
    if (!IsNumeric(rhsType)) {
      errors_->Append(MakeTypeMismatchError(TypeId::kInt, rhsType, ExtentOf(expr.RhsPtr())));
    }
    return nullptr;
  }

  if (IsEqualityOp(op)) {
    if (!IsComparable(lhsType, rhsType)) {
      errors_->Append(MakeIncomparableTypeError(lhsType, rhsType, expr.Op().pos));
      return nullptr;
    }
    return make_shared<BinExpr>(lhs, expr.Op(), rhs, TypeId::kBool);
  }

  const TypeId kStrType = typeset_.Get({"java", "lang", "String"});
  if (op == lexer::ADD && !kStrType.IsError() && (lhsType == kStrType || rhsType == kStrType)) {
    return make_shared<BinExpr>(lhs, expr.Op(), rhs, kStrType);
  }

  assert(IsNumericOp(op));
  if (IsNumeric(lhsType) && IsNumeric(rhsType)) {
    return make_shared<BinExpr>(lhs, expr.Op(), rhs, TypeId::kInt);
  }

  if (!IsNumeric(lhsType)) {
    errors_->Append(MakeTypeMismatchError(TypeId::kInt, lhsType, ExtentOf(expr.LhsPtr())));
  }
  if (!IsNumeric(rhsType)) {
    errors_->Append(MakeTypeMismatchError(TypeId::kInt, rhsType, ExtentOf(expr.RhsPtr())));
  }
  return nullptr;
}

REWRITE_DEFN(TypeChecker, BoolLitExpr, Expr, expr, ) {
  return make_shared<BoolLitExpr>(expr.GetToken(), TypeId::kBool);
}

// TODO: CallExpr

REWRITE_DEFN(TypeChecker, CastExpr, Expr, expr, exprptr) {
  sptr<const Expr> castedExpr = Rewrite(expr.GetExprPtr());
  sptr<const Type> type = MustResolveType(expr.GetTypePtr());
  if (castedExpr == nullptr || type == nullptr) {
    return nullptr;
  }
  TypeId exprType = castedExpr->GetTypeId();
  TypeId castType = type->GetTypeId();

  if (!IsCastable(castType, exprType)) {
    errors_->Append(MakeIncompatibleCastError(castType, exprType, ExtentOf(exprptr)));
    return nullptr;
  }

  return make_shared<CastExpr>(expr.Lparen(), type, expr.Rparen(), castedExpr, castType);
}

REWRITE_DEFN(TypeChecker, CharLitExpr, Expr, expr, ) {
  return make_shared<CharLitExpr>(expr.GetToken(), TypeId::kChar);
}

// TODO: FieldDerefExpr

REWRITE_DEFN(TypeChecker, InstanceOfExpr, Expr, expr, exprptr) {
  sptr<const Expr> lhs = Rewrite(expr.LhsPtr());
  sptr<const Type> rhs = MustResolveType(expr.GetTypePtr());
  if (lhs == nullptr || rhs == nullptr) {
    return nullptr;
  }
  TypeId lhsType = lhs->GetTypeId();
  TypeId rhsType = rhs->GetTypeId();

  if (IsPrimitive(lhsType) || IsPrimitive(rhsType)) {
    errors_->Append(MakeInstanceOfPrimitiveError(ExtentOf(exprptr)));
    return nullptr;
  }

  if (!IsCastable(lhsType, rhsType)) {
    errors_->Append(MakeIncompatibleInstanceOfError(lhsType, rhsType, ExtentOf(exprptr)));
    return nullptr;
  }

  return make_shared<InstanceOfExpr>(lhs, expr.InstanceOf(), rhs, TypeId::kBool);
}

REWRITE_DEFN(TypeChecker, IntLitExpr, Expr, expr, ) {
  return make_shared<IntLitExpr>(expr.GetToken(), expr.Value(), TypeId::kInt);
}

// TODO: NameExpr

REWRITE_DEFN(TypeChecker, NewArrayExpr, Expr, expr,) {
  sptr<const Type> elemtype = MustResolveType(expr.GetTypePtr());
  sptr<const Expr> index;
  if (expr.GetExprPtr() != nullptr) {
    index = Rewrite(expr.GetExprPtr());
  }

  if (elemtype == nullptr) {
    return nullptr;
  }

  // TODO: are we supposed to allow any numeric here?
  if (index != nullptr && index->GetTypeId() != TypeId::kInt) {
    errors_->Append(MakeTypeMismatchError(TypeId::kInt, index->GetTypeId(), ExtentOf(index)));
    return nullptr;
  }

  TypeId elem_tid = elemtype->GetTypeId();
  TypeId expr_tid = TypeId{elem_tid.base, elem_tid.ndims + 1};

  return make_shared<NewArrayExpr>(expr.NewToken(), elemtype, expr.Lbrack(), index, expr.Rbrack(), expr_tid);
}

REWRITE_DEFN(TypeChecker, NewClassExpr, Expr, expr, ) {
  // TODO: Lookup constructor with arg types.
  sptr<const Type> objType = MustResolveType(expr.GetTypePtr());
  return make_shared<NewClassExpr>(expr.NewToken(), expr.GetTypePtr(), expr.Lparen(), expr.Args(), expr.Rparen(), objType->GetTypeId());
}

REWRITE_DEFN(TypeChecker, NullLitExpr, Expr, expr, ) {
  return make_shared<NullLitExpr>(expr.GetToken(), TypeId::kNull);
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
  // TODO: this should only work in an instance context. i.e. we should only
  // populate curtype_ when entering a non-static method, or a non-static field
  // initializer.
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
    return make_shared<UnaryExpr>(expr.Op(), rhs, TypeId::kInt);
  }

  assert(op == lexer::NOT);
  TypeId expected = TypeId::kBool;
  if (rhsType != expected) {
    errors_->Append(MakeUnaryNonBoolError(rhsType, ExtentOf(exprptr)));
    return nullptr;
  }
  return make_shared<UnaryExpr>(expr.Op(), rhs, TypeId::kBool);
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

  TypeId expected = TypeId::kBool;
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

  TypeId expected = TypeId::kBool;
  if (cond->GetTypeId() != expected) {
    errors_->Append(MakeTypeMismatchError(expected, cond->GetTypeId(), ExtentOf(stmt.CondPtr())));
    return nullptr;
  }

  return make_shared<IfStmt>(cond, trueBody, falseBody);
}

REWRITE_DEFN(TypeChecker, LocalDeclStmt, Stmt, stmt,) {
  sptr<const Type> type = MustResolveType(stmt.GetTypePtr());
  sptr<const Expr> expr = Rewrite(stmt.GetExprPtr());

  if (type == nullptr || expr == nullptr) {
    return nullptr;
  }

  if (!IsAssignable(type->GetTypeId(), expr->GetTypeId())) {
    errors_->Append(MakeUnassignableError(type->GetTypeId(), expr->GetTypeId(), ExtentOf(expr)));
    return nullptr;
  }

  // TODO: put into symbol table, and assign local variable id.

  return make_shared<LocalDeclStmt>(type, stmt.Name(), stmt.NameToken(), expr);
}

REWRITE_DEFN(TypeChecker, ReturnStmt, Stmt, stmt,) {
  sptr<const Expr> expr = nullptr;
  if (stmt.GetExprPtr() != nullptr) {
    expr = Rewrite(stmt.GetExprPtr());
    if (expr == nullptr) {
      return nullptr;
    }
  }

  TypeId exprType = TypeId::kVoid;
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

  if (cond->GetTypeId() != TypeId::kBool) {
    errors_->Append(MakeTypeMismatchError(TypeId::kBool, cond->GetTypeId(), ExtentOf(stmt.CondPtr())));
    return nullptr;
  }

  return make_shared<WhileStmt>(cond, body);
}

REWRITE_DEFN(TypeChecker, FieldDecl, MemberDecl, decl,) {
  sptr<const Type> type = MustResolveType(decl.GetTypePtr());
  sptr<const Expr> val = nullptr;
  if (decl.ValPtr() != nullptr) {
    val = Rewrite(decl.ValPtr());
  }

  if (type == nullptr || (decl.ValPtr() != nullptr && val == nullptr)) {
    return nullptr;
  }

  if (val != nullptr) {
    if (!IsAssignable(type->GetTypeId(), val->GetTypeId())) {
      errors_->Append(MakeUnassignableError(type->GetTypeId(), val->GetTypeId(), ExtentOf(decl.ValPtr())));
      return nullptr;
    }
  }


  // TODO: When we start putting field-ids into FieldDecl, then this should
  // also populate it.

  return make_shared<FieldDecl>(decl.Mods(), type, decl.Name(), decl.NameToken(), val);
}

REWRITE_DEFN(TypeChecker, MethodDecl, MemberDecl, decl, declptr) {
  // If we have method info, then just use the default implementation of
  // RewriteMethodDecl.
  if (belowMethodDecl_) {
    return Visitor::RewriteMethodDecl(decl, declptr);
  }

  // Otherwise create a sub-visitor that has the method info, and let it
  // rewrite this node.

  TypeId rettype = TypeId::kVoid;
  if (decl.TypePtr() != nullptr) {
    rettype = decl.TypePtr()->GetTypeId();

    // This should have been pruned by previous pass if the type is invalid.
    assert(!rettype.IsError());
  }

  TypeChecker below = InsideMethodDecl(rettype);
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

  TypeChecker below = InsideTypeDecl(curtid);
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
  TypeChecker below = WithTypeSet(scopedTypeSet).InsideCompUnit(unit.PackagePtr());

  return below.Rewrite(unitptr);
}

} // namespace types
