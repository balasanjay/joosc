#ifndef TYPES_DECL_RESOLVER_H
#define TYPES_DECL_RESOLVER_H

#include "ast/visitor.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "types/type_info_map.h"
#include "types/types.h"

namespace types {

class DeclResolver : public ast::Visitor {
 public:
  DeclResolver(TypeInfoMapBuilder* builder, const TypeSet& typeset,
               const base::FileSet* fs, base::ErrorList* errors, sptr<const ast::QualifiedName> package = nullptr, ast::TypeId curtype = ast::TypeId::Unassigned())
      : builder_(builder), typeset_(typeset), fs_(fs), errors_(errors), package_(package), curtype_(curtype) {}

  REWRITE_DECL(FieldDecl, MemberDecl, args, argsptr);
  REWRITE_DECL(MethodDecl, MemberDecl, args, argsptr);
  REWRITE_DECL(ClassDecl, TypeDecl, args, argsptr);
  REWRITE_DECL(CompUnit, CompUnit, args, argsptr);

 private:
  ast::TypeId MustResolveType(const ast::Type& type);

  TypeInfoMapBuilder* builder_;
  const TypeSet& typeset_;
  const base::FileSet* fs_;
  base::ErrorList* errors_;
  sptr<const ast::QualifiedName> package_; // Only populated if below CompUnit and the CompUnit has a package statement.
  ast::TypeId curtype_; // Only populated if below TypeDecl.
};

}  // namespace types

#endif
