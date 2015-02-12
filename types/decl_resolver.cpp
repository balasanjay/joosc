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
using base::Error;
using base::FileSet;
using base::PosRange;
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

REWRITE_DEFN(DeclResolver, CompUnit, CompUnit, unit) {
  QualifiedName* package = nullptr;
  if (unit.Package() != nullptr) {
    package = new QualifiedName(*unit.Package());
  }

  TypeSet scopedTypeSet = typeset_.WithImports(unit.Imports());
  DeclResolver scopedResolver(builder_, scopedTypeSet, fs_, errors_, package);

  base::UniquePtrVector<TypeDecl> decls;
  for (const auto& type : unit.Types()) {
    TypeDecl* newtype = type.AcceptRewriter(&scopedResolver);
    if (newtype != nullptr) {
      decls.Append(newtype);
    }
  }
  return new CompUnit(package, unit.Imports(), std::move(decls));
}

REWRITE_DEFN(DeclResolver, ClassDecl, TypeDecl, type) {
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
  ModifierList mods(type.Mods());
  base::UniquePtrVector<MemberDecl> members;
  for (const auto& member : type.Members()) {
    MemberDecl* decl = member.AcceptRewriter(&memberResolver);
    if (decl != nullptr) {
      members.Append(decl);
    }
  }
  ReferenceType* super = nullptr;
  if (type.Super() != nullptr) {
    super = static_cast<ReferenceType*>(type.Super()->Clone());
  }
  return new ClassDecl(std::move(mods), type.Name(), type.NameToken(), type.Interfaces(), std::move(members), super, curtid);
}

// TODO: we are skipping interfaces; waiting until after the AST merge.
// TODO: we are skipping constructors; waiting until after the AST merge.

REWRITE_DEFN(DeclResolver, FieldDecl, MemberDecl, field) {
  TypeId tid = MustResolveType(field.GetType());
  if (tid.IsError()) {
    return nullptr;
  }

  // TODO: put field in table keyed by (curtid_, field.Name()).
  // TODO: assign member id to field.

  ModifierList mods(field.Mods());
  Type* type = field.GetType().Clone();
  Expr* val = nullptr;
  if (field.Val() != nullptr) {
    val = field.Val()->AcceptRewriter(this);
  }
  return new FieldDecl(std::move(mods), type, field.Ident(), val);
}

REWRITE_DEFN(DeclResolver, MethodDecl, MemberDecl, meth) {
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

  ModifierList mods(meth.Mods());
  Type* type = meth.GetType().Clone();
  uptr<ParamList> params(meth.Params().AcceptRewriter(this));
  Stmt* body = meth.Body().AcceptRewriter(this);
  return new MethodDecl(std::move(mods), type, meth.Ident(), std::move(*params), body);
}

} // namespace types
