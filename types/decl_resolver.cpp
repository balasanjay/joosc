#include "types/decl_resolver.h"

#include "ast/ast.h"
#include "base/error.h"
#include "base/macros.h"
#include "types/types_internal.h"

using ast::ArrayType;
using ast::CompUnit;
using ast::Expr;
using ast::FieldDecl;
using ast::MemberDecl;
using ast::MethodDecl;
using ast::ModifierList;
using ast::Param;
using ast::ParamList;
using ast::PrimitiveType;
using ast::QualifiedName;
using ast::ReferenceType;
using ast::Stmt;
using ast::Type;
using ast::TypeDecl;
using ast::TypeId;
using ast::TypeKind;
using base::Error;
using base::FileSet;
using base::PosRange;
using base::SharedPtrVector;
using base::UniquePtrVector;

namespace types {

sptr<const Type> DeclResolver::MustResolveType(sptr<const Type> type) {
  PosRange pos(-1, -1, -1);
  sptr<const Type> ret = ResolveType(type, typeset_, &pos);
  if (ret->GetTypeId().IsUnassigned()) {
    errors_->Append(MakeUnknownTypenameError(fs_, pos));
  }
  return ret;
}

REWRITE_DEFN(DeclResolver, CompUnit, CompUnit, unit,) {
  TypeSet scopedTypeSet = typeset_.WithImports(unit.Imports(), errors_);
  DeclResolver scopedResolver(builder_, scopedTypeSet, fs_, errors_, unit.PackagePtr());

  base::SharedPtrVector<const TypeDecl> decls;
  for (int i = 0; i < unit.Types().Size(); ++i) {
    sptr<const TypeDecl> oldtype = unit.Types().At(i);
    sptr<const TypeDecl> newtype = scopedResolver.Rewrite(oldtype);
    if (newtype != nullptr) {
      decls.Append(newtype);
    }
  }
  return make_shared<CompUnit>(unit.PackagePtr(), unit.Imports(), decls);
}

REWRITE_DEFN(DeclResolver, TypeDecl, TypeDecl, type, ) {
  // Try and resolve TypeId of this class. If this fails, that means that this
  // class has some previously discovered error, and we prune this subtree.
  vector<string> classname;
  if (package_ != nullptr) {
    classname = package_->Parts();
  }
  classname.push_back(type.Name());

  TypeId curtid = typeset_.Get(classname);
  if (!curtid.IsValid()) {
    return nullptr;
  }

  // A helper function to build the extends and implements lists.
  const auto AddToTypeIdVector = [&](const QualifiedName& name, vector<TypeId>* out) {
    TypeId tid = typeset_.Get(name.Parts());
    if (tid.IsValid()) {
      out->push_back(tid);
      return;
    }

    // If we've never heard about this type before, then emit an error.
    if (tid.IsUnassigned()) {
      errors_->Append(MakeUnknownTypenameError(fs_, name.Tokens().back().pos));
      return;
    }

    // This is a type we've already emitted an error about, so don't emit one again.
    assert(tid.IsError());
  };

  vector<TypeId> extends;
  for (const auto& name : type.Extends()) {
    AddToTypeIdVector(name, &extends);
  }

  vector<TypeId> implements;
  for (const auto& name : type.Implements()) {
    AddToTypeIdVector(name, &implements);
  }

  // TODO: put into TypeInfoMapBuilder.
  DeclResolver memberResolver(builder_, typeset_, fs_, errors_, package_, curtid);
  SharedPtrVector<const MemberDecl> members;
  for (int i = 0; i < type.Members().Size(); ++i) {
    sptr<const MemberDecl> oldMem = type.Members().At(i);
    sptr<const MemberDecl> newMem = memberResolver.Rewrite(oldMem);
    if (newMem != nullptr) {
      members.Append(newMem);
    }
  }
  return make_shared<TypeDecl>(type.Mods(), type.Kind(), type.Name(), type.NameToken(), type.Extends(), type.Implements(), members, curtid);
}

REWRITE_DEFN(DeclResolver, FieldDecl, MemberDecl, field, ) {
  sptr<const Type> type = MustResolveType(field.GetTypePtr());
  if (!type->GetTypeId().IsValid()) {
    return nullptr;
  }

  // TODO: put field in table keyed by (curtid_, field.Name()).
  // TODO: assign member id to field.
  return make_shared<FieldDecl>(field.Mods(), type, field.Name(), field.NameToken(), field.ValPtr());
}

REWRITE_DEFN(DeclResolver, MethodDecl, MemberDecl, meth,) {
  sptr<const Type> ret_type = nullptr;
  TypeId rettid = TypeId::kUnassigned;
  if (meth.TypePtr() == nullptr) {
    // Handle constructor.
    // The return type of a constructor is the containing class.
    rettid = curtype_;
  } else {
    // Handle method.
    ret_type = MustResolveType(meth.TypePtr());
    rettid = ret_type->GetTypeId();
  }

  SharedPtrVector<const Param> params;
  vector<TypeId> paramtids;
  for (const auto& param : meth.Params().Params()) {
    sptr<const Type> paramType = MustResolveType(param.GetTypePtr());
    if (paramType->GetTypeId().IsValid()) {
      paramtids.push_back(paramType->GetTypeId());
      params.Append(make_shared<Param>(paramType, param.Name(), param.NameToken()));
    }
  }

  if (!rettid.IsValid() || params.Size() < meth.Params().Params().Size()) {
    return nullptr;
  }

  // TODO: put method in table keyed by (curtid_, meth.Name(), paramtids).
  // TODO: assign member id to method.

  return make_shared<MethodDecl>(meth.Mods(), ret_type, meth.Name(),
      meth.NameToken(), make_shared<ParamList>(params), meth.BodyPtr());
}

} // namespace types
