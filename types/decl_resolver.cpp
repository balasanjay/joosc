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
  return ResolveType(type, typeset_, errors_);
}

REWRITE_DEFN(DeclResolver, CompUnit, CompUnit, unit,) {
  // TODO: if the typeset.WithImports finds an error in the imports, we should
  // trim the imports, so that subsequent passes do not emit the same error.
  TypeSet scoped_typeset  = typeset_
      .WithPackage(unit.PackagePtr(), errors_)
      .WithImports(unit.Imports(), errors_);;

  DeclResolver scoped_resolver(builder_, scoped_typeset, fs_, errors_, unit.PackagePtr());

  base::SharedPtrVector<const TypeDecl> decls;
  for (int i = 0; i < unit.Types().Size(); ++i) {
    sptr<const TypeDecl> oldtype = unit.Types().At(i);
    sptr<const TypeDecl> newtype = scoped_resolver.Rewrite(oldtype);
    if (newtype != nullptr) {
      decls.Append(newtype);
    }
  }
  return make_shared<CompUnit>(unit.PackagePtr(), unit.Imports(), decls);
}

REWRITE_DEFN(DeclResolver, TypeDecl, TypeDecl, type, ) {
  // First, fetch a nested TypeSet for this type.
  TypeSet scoped_resolver = typeset_.WithType(type.Name(), type.NameToken().pos, errors_);

  // Then, try and resolve TypeId of this class. If this fails, that means that
  // this class has some serious previously discovered error (cycles in the
  // import graph, for example). We immediately prune the subtree.
  TypeId curtid = typeset_.TryGet(type.Name());
  if (!curtid.IsValid()) {
    return nullptr;
  }

  // A helper function to build the extends and implements lists.
  const auto AddToTypeIdVector = [&](const QualifiedName& name, vector<TypeId>* out) {
    PosRange pos = name.Tokens().front().pos;
    pos.end = name.Tokens().back().pos.end;

    TypeId tid = typeset_.Get(name.Name(), pos, errors_);
    if (tid.IsValid()) {
      out->push_back(tid);
    }
  };

  vector<TypeId> extends;
  for (const auto& name : type.Extends()) {
    AddToTypeIdVector(name, &extends);
  }

  vector<TypeId> implements;
  for (const auto& name : type.Implements()) {
    AddToTypeIdVector(name, &implements);
  }

  builder_->PutType(curtid, type, extends, implements);

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
  bool is_constructor = false;
  if (meth.TypePtr() == nullptr) {
    // Handle constructor.
    // The return type of a constructor is the containing class.
    rettid = curtype_;
    is_constructor = true;
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

  builder_->PutMethod(curtype_, rettid, paramtids, meth, is_constructor);
  // TODO: assign member id to method.

  return make_shared<MethodDecl>(meth.Mods(), ret_type, meth.Name(),
      meth.NameToken(), make_shared<ParamList>(params), meth.BodyPtr());
}

} // namespace types
