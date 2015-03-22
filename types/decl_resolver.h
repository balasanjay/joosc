#ifndef TYPES_DECL_RESOLVER_H
#define TYPES_DECL_RESOLVER_H

#include "ast/visitor.h"
#include "base/errorlist.h"
#include "types/type_info_map.h"
#include "types/typeset.h"

namespace types {

class DeclResolver : public ast::Visitor {
 public:
  DeclResolver(TypeInfoMapBuilder* builder, const TypeSet& typeset,
               base::ErrorList* errors, sptr<const ast::QualifiedName> package = nullptr, ast::TypeId curtype = ast::TypeId::kUnassigned)
      : builder_(builder), typeset_(typeset), errors_(errors), package_(package), curtype_(curtype) {}

  REWRITE_DECL(FieldDecl, MemberDecl, args, argsptr);
  REWRITE_DECL(MethodDecl, MemberDecl, args, argsptr);
  REWRITE_DECL(TypeDecl, TypeDecl, args, argsptr);
  REWRITE_DECL(CompUnit, CompUnit, args, argsptr);

 private:
  sptr<const ast::Type> MustResolveType(sptr<const ast::Type> type);

  TypeInfoMapBuilder* builder_;
  const TypeSet& typeset_;
  base::ErrorList* errors_;
  sptr<const ast::QualifiedName> package_; // Only populated if below CompUnit and the CompUnit has a package statement.
  ast::TypeId curtype_; // Only populated if below TypeDecl.
};

}  // namespace types

#endif
