#include "types/typechecker.h"

#include "ast/ast.h"
#include "ast/extent.h"
#include "base/error.h"
#include "base/macros.h"
#include "types/types_internal.h"
#include "types/symbol_table.h"

using namespace ast;

using base::Error;
using base::ErrorList;
using base::FileSet;
using base::Pos;
using base::PosRange;
using base::SharedPtrVector;
using lexer::K_THIS;
using lexer::Token;
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
    if (!IsAssignable(lhsType, rhsType)) {
      errors_->Append(MakeUnassignableError(lhsType, rhsType, ExtentOf(rhs)));
      return nullptr;
    }
    return make_shared<BinExpr>(lhs, expr.Op(), rhs, lhsType);
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

  const TypeId kStrType = JavaLangType("String");
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

REWRITE_DEFN(TypeChecker, FieldDerefExpr, Expr, expr,) {
  sptr<const Expr> base = Rewrite(expr.BasePtr());
  if (base == nullptr) {
    return nullptr;
  }
  const TypeInfo& tinfo = typeinfo_.LookupTypeInfo(base->GetTypeId());
  CallContext cc = CallContext::INSTANCE;
  if (dynamic_cast<const StaticRefExpr*>(base.get()) != nullptr) {
    cc = CallContext::STATIC;
  }
  FieldId fid = tinfo.fields.ResolveAccess(curtype_, cc, expr.FieldName(), expr.GetToken().pos, errors_);
  if (fid == kErrorFieldId) {
    return nullptr;
  }
  FieldInfo finfo = tinfo.fields.LookupField(fid);
  return make_shared<FieldDerefExpr>(base, expr.FieldName(), expr.GetToken(), fid, finfo.field_type);
}

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

REWRITE_DEFN(TypeChecker, IntLitExpr, Expr, expr,) {
  return make_shared<IntLitExpr>(expr.GetToken(), expr.Value(), TypeId::kInt);
}

sptr<const Expr> SplitQualifiedToFieldDerefs(
    sptr<const Expr> base, const QualifiedName& name, int start_idx) {
  const vector<string> parts = name.Parts();
  const vector<Token> toks = name.Tokens();
  assert(start_idx > 0);

  // If they want to split past-the-end, then just use the base.
  if ((uint)start_idx == parts.size()) {
    return base;
  }

  assert((uint)start_idx < parts.size());

  // Parts includes the dots between tokens; quickly validate this assumption
  // holds.
  assert(((parts.size() - 1) * 2) + 1 == toks.size());

  sptr<const Expr> cur_base = base;
  for (uint i = (uint)start_idx; i < parts.size(); ++i) {
    sptr<const Expr> field_deref = make_shared<FieldDerefExpr>(cur_base, parts.at(i), toks.at(2 * i));
    cur_base = field_deref;
  }
  return cur_base;
}

QualifiedName SliceFirstN(const QualifiedName& name, int n) {
  const vector<string>& old_parts = name.Parts();
  const vector<Token>& old_toks = name.Tokens();

  assert(n > 0);

  vector<string> parts;
  vector<Token> toks;
  stringstream fullname;

  parts.push_back(old_parts.at(0));
  toks.push_back(old_toks.at(0));
  fullname << old_parts.at(0);
  for (uint i = 1; i < (uint)n; ++i) {
    parts.push_back(old_parts.at(i));
    toks.push_back(old_toks.at((2*i) - 1));
    toks.push_back(old_toks.at((2*i)));
    fullname << '.' << old_parts.at(i);
  }

  return QualifiedName(toks, parts, fullname.str());
}

sptr<const Expr> MakeImplicitThis(PosRange pos, TypeId tid) {
  return make_shared<ThisExpr>(Token(K_THIS, Pos(pos.fileid, pos.begin)), tid);
}

REWRITE_DEFN(TypeChecker, NameExpr, Expr, expr, exprptr) {
  // If we've already assigned the vid, then we've resolved this node before
  // when disambiguating a qualified name.
  if (expr.GetVarId() != kVarUnassigned) {
    return exprptr;
  }

  const vector<string> parts = expr.Name().Parts();
  const vector<Token> toks = expr.Name().Tokens();
  assert(parts.size() > 0);
  assert(belowTypeDecl_);

  // First, try resolving it as a local variable or a param.
  {
    // We don't bother using the local var decl error, because the field error
    // will be strictly superior.
    ErrorList throwaway;
    pair<TypeId, ast::LocalVarId> var_data = symbol_table_.ResolveLocal(
        parts.at(0), toks.at(0).pos, &throwaway);
    bool ok = (var_data.first != TypeId::kUnassigned && var_data.second != kVarUnassigned);

    // If the local resolved successfully, we split the current NameExpr into a
    // series of FieldDerefs, and recurse on it.
    if (ok) {
      sptr<const Expr> name_expr = make_shared<NameExpr>(SliceFirstN(expr.Name(), 1), var_data.second, var_data.first);
      return Rewrite(SplitQualifiedToFieldDerefs(name_expr, expr.Name(), 1));
    }
  }

  // Next, try resolving it as a field. We keep any emitted errors around. We
  // might use them if resolving this as a Type fails.
  ErrorList field_errors;
  {
    // TODO: add a field lookup in here.
    TypeInfo tinfo = typeinfo_.LookupTypeInfo(curtype_);
    FieldId fid = tinfo.fields.ResolveAccess(curtype_, CallContext::INSTANCE, parts.at(0), toks.at(0).pos, &field_errors);
    bool ok = fid != kErrorFieldId;
    if (ok) {
      sptr<const Expr> implicit_this = MakeImplicitThis(toks.at(0).pos, curtype_);
      sptr<const Expr> field_deref = make_shared<FieldDerefExpr>(implicit_this, parts.at(0), toks.at(0));
      return Rewrite(SplitQualifiedToFieldDerefs(field_deref, expr.Name(), 1));
    }
  }

  // Last, try looking up successive prefixes as a type.
  {
    stringstream ss;
    for (uint i = 0; i < parts.size(); ++i) {
      if (i != 0) {
        ss << '.';
      }
      ss << parts.at(i);
      string name = ss.str();
      TypeId tid = typeset_.TryGet(name);
      if (tid.IsValid()) {
        sptr<const Type> resolved_type = make_shared<ReferenceType>(
            SliceFirstN(expr.Name(), i + 1), tid);
        auto static_ref = make_shared<StaticRefExpr>(resolved_type);
        return Rewrite(SplitQualifiedToFieldDerefs(static_ref, expr.Name(), i + 1));
      }
    }
  }

  // Release the field errors, and prune the rest.
  vector<Error*> released;
  field_errors.Release(&released);
  for (auto err : released) {
    errors_->Append(err);
  }
  released.clear();
  return nullptr;
}

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

REWRITE_DEFN(TypeChecker, NullLitExpr, Expr, expr, ) {
  return make_shared<NullLitExpr>(expr.GetToken(), TypeId::kNull);
}

REWRITE_DEFN(TypeChecker, ParenExpr, Expr, expr,) {
  return Rewrite(expr.NestedPtr());
}

REWRITE_DEFN(TypeChecker, StringLitExpr, Expr, expr,) {
  const TypeId strType = JavaLangType("String");
  if (strType.IsError()) {
    errors_->Append(MakeNoStringError(expr.GetToken().pos));
    return nullptr;
  }

  return make_shared<StringLitExpr>(expr.GetToken(), strType);
}

REWRITE_DEFN(TypeChecker, ThisExpr, Expr, expr,) {
  if (belowStaticMember_) {
    errors_->Append(MakeThisInStaticMemberError(expr.ThisToken().pos));
    return nullptr;
  }
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

REWRITE_DEFN(TypeChecker, BlockStmt, Stmt, stmt, stmtptr) {
  ScopeGuard s(&symbol_table_);
  return Visitor::RewriteBlockStmt(stmt, stmtptr);
}

REWRITE_DEFN(TypeChecker, ForStmt, Stmt, stmt,) {
  // Enter scope for decls in for init.
  ScopeGuard s(&symbol_table_);

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
  sptr<const Expr> expr;

  LocalVarId vid = kVarUnassigned;
  TypeId tid = TypeId::kUnassigned;

  // Assign variable even if type lookup fails so we don't show undefined reference errors.
  if (type != nullptr) {
    tid = type->GetTypeId();
  }

  {
    VarDeclGuard g(&symbol_table_, tid, stmt.Name(), stmt.NameToken().pos, errors_);
    expr = Rewrite(stmt.GetExprPtr());
    vid = g.GetVarId();
  }

  if (type == nullptr || expr == nullptr) {
    return nullptr;
  }

  if (!IsAssignable(type->GetTypeId(), expr->GetTypeId())) {
    errors_->Append(MakeUnassignableError(type->GetTypeId(), expr->GetTypeId(), ExtentOf(expr)));
    return nullptr;
  }

  return make_shared<LocalDeclStmt>(type, stmt.Name(), stmt.NameToken(), expr, vid);
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

  assert(belowMemberDecl_);
  if (!IsAssignable(curMemberType_, exprType)) {
    errors_->Append(MakeInvalidReturnError(curMemberType_, exprType, stmt.ReturnToken().pos));
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

REWRITE_DEFN(TypeChecker, FieldDecl, MemberDecl, decl, declptr) {
  // If we have method info, then just use the default implementation of
  // RewriteMethodDecl.
  if (!belowMemberDecl_) {
    bool is_static = decl.Mods().HasModifier(lexer::Modifier::STATIC);
    TypeChecker below = InsideMemberDecl(is_static);
    return below.RewriteFieldDecl(decl, declptr);
  }

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

  // Lookup field and rewrite with fid.
  const TypeInfo& tinfo = typeinfo_.LookupTypeInfo(curtype_);
  FieldInfo finfo = tinfo.fields.LookupField(decl.Name());

  return make_shared<FieldDecl>(decl.Mods(), type, decl.Name(), decl.NameToken(), val, finfo.fid);
}

REWRITE_DEFN(TypeChecker, MethodDecl, MemberDecl, decl, declptr) {
  // If we have method info, then just use the default implementation of
  // RewriteMethodDecl.
  if (belowMemberDecl_) {
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

  bool is_static = decl.Mods().HasModifier(lexer::Modifier::STATIC);
  TypeChecker below = InsideMemberDecl(is_static, rettype, decl.Params());
  return below.Rewrite(declptr);
}

// Rewrite params to include the local var ids that were just assigned to them.
REWRITE_DEFN(TypeChecker, Param, Param, param,) {
  LocalVarId vid;
  std::tie(std::ignore, vid) = symbol_table_.ResolveLocal(param.Name(), param.NameToken().pos, errors_);
  assert(vid != kVarUnassigned);
  return make_shared<Param>(param.GetTypePtr(), param.Name(), param.NameToken(), vid);
}

REWRITE_DEFN(TypeChecker, TypeDecl, TypeDecl, type, typeptr) {
  // If we have method info, then just use the default implementation of
  // RewriteTypeDecl.
  if (belowTypeDecl_) {
    return Visitor::RewriteTypeDecl(type, typeptr);
  }

  // Otherwise create a sub-visitor that has the type info, and let it rewrite
  // this node.

  TypeSet scoped_typeset = typeset_.WithType(type.Name(), type.NameToken().pos, errors_);
  TypeId curtid = scoped_typeset.TryGet(type.Name());
  assert(!curtid.IsError()); // Pruned in DeclResolver.

  TypeChecker below = InsideTypeDecl(curtid, scoped_typeset);
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
  TypeSet scoped_typeset = typeset_
      .WithPackage(unit.PackagePtr(), errors_)
      .WithImports(unit.Imports(), errors_);

  TypeChecker below = (*this)
      .WithTypeSet(scoped_typeset)
      .InsideCompUnit(unit.PackagePtr());

  return below.Rewrite(unitptr);
}

// TODO: Override Program. Use that to lookup java.lang.{String, Boolean,
// Integer,...} and emit an error if they're missing. Then we can set their
// TypeIds as fields on a nested TypeChecker, and always use those fields
// directly. We'll still have to check if the fields are Valid(), but we won't
// have to emit an error. Note that we won't have a PosRange to complain about,
// so it'll have to a custom Error. See the MakeError() function in
// base/error.h.

} // namespace types
