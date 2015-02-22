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
using base::UniquePtrVector;

namespace types {

TypeId TypeChecker::MustResolveType(const Type& type) {
  PosRange pos(-1, -1, -1);
  TypeId tid = ResolveType(type, typeset_, &pos);
  if (tid.IsError()) {
    errors_->Append(MakeUnknownTypenameError(fs_, pos));
  }
  return tid;
}

Error* TypeChecker::MakeTypeMismatchError(TypeId expected, TypeId got, PosRange pos) {
  // TODO: lookup expected and got in typeinfo_ and get a name.
  stringstream ss;
  ss << "Type mismatch; expected " << expected.base << ", got " << got.base;
  return MakeSimplePosRangeError(fs_, pos, "TypeMismatchError", ss.str());
}

Error* TypeChecker::MakeIndexNonArrayError(PosRange pos) {
  return MakeSimplePosRangeError(fs_, pos, "IndexNonArrayError", "Cannot index non-array.");
}

REWRITE_DEFN(TypeChecker, CompUnit, CompUnit, unit, unitptr) {
  if (belowCompUnit_) {
    return Visitor::RewriteCompUnit(unit, unitptr);
  }

  TypeSet scopedTypeSet = typeset_.WithImports(unit.Imports(), fs_, errors_);
  TypeChecker below(typeinfo_, scopedTypeSet, fs_, errors_, true, unit.PackagePtr());

  return below.Visit(unitptr);
}

REWRITE_DEFN(TypeChecker, TypeDecl, TypeDecl, type, typeptr) {
  if (belowTypeDecl_) {
    return Visitor::RewriteTypeDecl(type, typeptr);
  }

  vector<string> classname;
  if (package_ != nullptr) {
    classname = package_->Parts();
  }
  classname.push_back(type.Name());

  TypeId curtid = typeset_.Get(classname);
  assert(!curtid.IsError()); // Pruned in DeclResolver.

  TypeChecker below(typeinfo_, typeset_, fs_, errors_, true, package_, true, curtid);
  return below.Visit(typeptr);
}

REWRITE_DEFN(TypeChecker, IntLitExpr, Expr, expr, ) {
  return make_shared<IntLitExpr>(expr.GetToken(), expr.Value(), TypeId{TypeId::kIntBase, 0});
}

REWRITE_DEFN(TypeChecker, ThisExpr, Expr, expr,) {
  return make_shared<ThisExpr>(expr.ThisToken(), curtype_);
}

REWRITE_DEFN(TypeChecker, NewArrayExpr, Expr, expr,) {
  TypeId tid = MustResolveType(expr.GetType());
  if (tid.IsError()) {
    return nullptr;
  }

  sptr<const Expr> index;
  TypeId expectedIndexType = TypeId{TypeId::kIntBase, 0};

  if (expr.GetExprPtr() != nullptr) {
    index = Visit(expr.GetExprPtr());
  }
  if (index != nullptr && index->GetTypeId() != expectedIndexType) {
    errors_->Append(MakeTypeMismatchError(expectedIndexType, index->GetTypeId(), ExtentOf(index)));
    return nullptr;
  }

  return make_shared<NewArrayExpr>(expr.NewToken(), expr.GetTypePtr(), expr.Lbrack(), index, expr.Rbrack(), TypeId{tid.base, tid.ndims + 1});
}

REWRITE_DEFN(TypeChecker, ArrayIndexExpr, Expr, expr,) {
  sptr<const Expr> base = Visit(expr.BasePtr());
  sptr<const Expr> index = Visit(expr.IndexPtr());
  if (base == nullptr || index == nullptr) {
    return nullptr;
  }

  TypeId expectedIndexType = TypeId{TypeId::kIntBase, 0};
  if (index->GetTypeId() != expectedIndexType) {
    errors_->Append(MakeTypeMismatchError(expectedIndexType, index->GetTypeId(), ExtentOf(index)));
    return nullptr;
  }
  if (base->GetTypeId().ndims < 1) {
    errors_->Append(MakeIndexNonArrayError(ExtentOf(base)));
    return nullptr;
  }

  TypeId tid = TypeId{base->GetTypeId().base, base->GetTypeId().ndims - 1};
  return make_shared<ArrayIndexExpr>(base, expr.Lbrack(), index, expr.Rbrack(), tid);
}


} // namespace types
