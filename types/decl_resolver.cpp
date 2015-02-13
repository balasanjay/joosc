#include "types/decl_resolver.h"

#include "ast/ast.h"
#include "base/error.h"
#include "base/macros.h"

using ast::ArrayType;
using ast::ClassDecl;
using ast::CompUnit;
using ast::ConstructorDecl;
using ast::Expr;
using ast::FieldDecl;
using ast::MemberDecl;
using ast::MethodDecl;
using ast::ModifierList;
using ast::ParamList;
using ast::PrimitiveType;
using ast::QualifiedName;
using ast::ReferenceType;
using ast::ReferenceType;
using ast::Stmt;
using ast::Type;
using ast::TypeDecl;
using ast::TypeId;
using ast::Visit;
using base::Error;
using base::FileSet;
using base::PosRange;
using base::SharedPtrVector;
using base::UniquePtrVector;

namespace types {

namespace {

Error* MakeUnknownTypenameError(const FileSet* fs, PosRange pos) {
  return MakeSimplePosRangeError(fs, pos, "UnknownTypenameError",
                                 "Unknown type name.");
}

TypeId ResolveType(const Type& type, TypeSet typeset, PosRange* pos_out) {
  const Type* cur = &type;
  if (IS_CONST_PTR(ReferenceType, cur)) {
    const ReferenceType* ref = dynamic_cast<const ReferenceType*>(cur);
    *pos_out = ref->Name().Tokens().back().pos;
    return typeset.Get(ref->Name().Parts());
  } else if (IS_CONST_PTR(PrimitiveType, cur)) {
    const PrimitiveType* prim = dynamic_cast<const PrimitiveType*>(cur);
    *pos_out = prim->GetToken().pos;
    return typeset.Get({prim->GetToken().TypeInfo().Value()});
  }

  assert(IS_CONST_PTR(ArrayType, cur));
  const ArrayType* arr = dynamic_cast<const ArrayType*>(cur);
  TypeId nested = ResolveType(arr->ElemType(), typeset, pos_out);
  return TypeId{nested.base, nested.ndims + 1};
}


} // namespace

TypeId DeclResolver::MustResolveType(const Type& type) {
  PosRange pos(-1, -1, -1);
  TypeId tid = ResolveType(type, typeset_, &pos);
  if (tid.IsError()) {
    errors_->Append(MakeUnknownTypenameError(fs_, pos));
  }
  return tid;
}

REWRITE_DEFN2(DeclResolver, CompUnit, CompUnit, unit,) {
  sptr<QualifiedName> package = nullptr;
  if (unit.PackagePtr() != nullptr) {
    package = unit.PackagePtr();
  }

  TypeSet scopedTypeSet = typeset_.WithImports(unit.Imports());
  DeclResolver scopedResolver(builder_, scopedTypeSet, fs_, errors_, package);

  base::SharedPtrVector<TypeDecl> decls;
  for (int i = 0; i < unit.Types().Size(); ++i) {
    sptr<TypeDecl> oldtype = unit.Types().At(i);
    sptr<TypeDecl> newtype = Visit(&scopedResolver, oldtype);
    if (newtype != nullptr) {
      decls.Append(newtype);
    }
  }
  return make_shared<CompUnit>(package, unit.Imports(), decls);
}

REWRITE_DEFN2(DeclResolver, ClassDecl, TypeDecl, type, ) {
  // Try and resolve TypeId of this class. If this fails, that means that this
  // class has some previously discovered error, and we prune this subtree.
  vector<string> classname;
  if (package_ != nullptr) {
    classname = package_->Parts();
  }
  classname.push_back(type.Name());

  TypeId curtid = typeset_.Get(classname);
  if (curtid.IsError()) {
    return nullptr;
  }

  vector<TypeId> interfaces;
  for (const auto& iface : type.Interfaces()) {
    TypeId tid = typeset_.Get(iface.Parts());
    if (!tid.IsError()) {
      interfaces.push_back(tid);
      continue;
    }

    errors_->Append(MakeUnknownTypenameError(fs_, iface.Tokens().back().pos));
  }

  // TODO: handle super.
  // TODO: put into builder_.
  DeclResolver memberResolver(builder_, typeset_, fs_, errors_, package_, curtid);
  SharedPtrVector<MemberDecl> members;
  for (int i = 0; i < type.Members().Size(); ++i) {
    sptr<MemberDecl> oldMem = type.Members().At(i);
    sptr<MemberDecl> newMem = Visit(&memberResolver, oldMem);
    if (newMem != nullptr) {
      members.Append(newMem);
    }
  }
  return make_shared<ClassDecl>(type.Mods(), type.Name(), type.NameToken(), type.Interfaces(), members, type.SuperPtr(), curtid);
}

// TODO: we are skipping interfaces; waiting until after the AST merge.
// TODO: we are skipping constructors; waiting until after the AST merge.

REWRITE_DEFN2(DeclResolver, FieldDecl, MemberDecl, field, ) {
  TypeId tid = MustResolveType(field.GetType());
  if (tid.IsError()) {
    return nullptr;
  }

  // TODO: put field in table keyed by (curtid_, field.Name()).
  // TODO: assign member id to field.
  return make_shared<FieldDecl>(field.Mods(), field.GetTypePtr(), field.Ident(), field.ValPtr());
}

REWRITE_DEFN2(DeclResolver, MethodDecl, MemberDecl, meth,) {
  TypeId rettid = MustResolveType(meth.GetType());
  if (rettid.IsError()) {
    return nullptr;
  }

  vector<TypeId> paramtids;
  for (const auto& param : meth.Params().Params()) {
    TypeId tid = MustResolveType(param.GetType());
    if (tid.IsError()) {
      return nullptr;
    }
    paramtids.push_back(tid);
  }

  // TODO: put method in table keyed by (curtid_, meth.Name(), paramtids).
  // TODO: assign member id to method.

  return make_shared<MethodDecl>(meth.Mods(), meth.GetTypePtr(), meth.Ident(), meth.ParamsPtr(), meth.BodyPtr());
}

} // namespace types
