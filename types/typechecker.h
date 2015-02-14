#ifndef TYPES_TYPECHECKER_H
#define TYPES_TYPECHECKER_H

#include "ast/visitor.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "types/type_info_map.h"
#include "types/typeset.h"

namespace types {

class TypeChecker final : public ast::Visitor {
 public:
  TypeChecker(const TypeInfoMap& typeinfo, const TypeSet& typeset,
              const base::FileSet* fs, base::ErrorList* errors,
              bool belowCompUnit = false, sptr<const ast::QualifiedName> package = nullptr,
              bool belowTypeDecl = false, ast::TypeId curtype = ast::TypeId::Unassigned())
      : typeinfo_(typeinfo), typeset_(typeset), fs_(fs), errors_(errors),
        belowCompUnit_(belowCompUnit), package_(package),
        belowTypeDecl_(belowTypeDecl), curtype_(curtype) {}

  REWRITE_DECL(TypeDecl, TypeDecl, args, argsptr);
  REWRITE_DECL(CompUnit, CompUnit, args, argsptr);

  REWRITE_DECL(IntLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(ThisExpr, Expr, expr, exprptr);
  REWRITE_DECL(NewArrayExpr, Expr, expr, exprptr);
  REWRITE_DECL(ArrayIndexExpr, Expr, expr, exprptr);

 private:
  ast::TypeId MustResolveType(const ast::Type& type);

  base::Error* MakeTypeMismatchError(ast::TypeId expected, ast::TypeId got, base::PosRange pos);
  base::Error* MakeIndexNonArrayError(base::PosRange pos);

  const TypeInfoMap& typeinfo_;
  const TypeSet& typeset_;
  const base::FileSet* fs_;
  base::ErrorList* errors_;

  const bool belowCompUnit_ = false;
  const sptr<const ast::QualifiedName> package_; // Only populated if below CompUnit and the CompUnit has a package statement.

  const bool belowTypeDecl_ = false;
  const ast::TypeId curtype_; // Only populated if below TypeDecl.
};

}  // namespace types

#endif
