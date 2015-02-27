#ifndef TYPES_TYPECHECKER_H
#define TYPES_TYPECHECKER_H

#include "third_party/gtest/gtest.h"
#include "ast/visitor.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "types/type_info_map.h"
#include "types/typeset.h"

namespace types {

class TypeChecker final : public ast::Visitor {
 public:
   TypeChecker(const base::FileSet* fs, base::ErrorList* errors) : TypeChecker(fs, errors, TypeSet::Empty(), TypeInfoMap::Empty()) {}

  TypeChecker WithTypeSet(const TypeSet& typeset) const {
    assert(!belowCompUnit_);
    return TypeChecker(fs_, errors_, typeset, typeinfo_);
  }

  TypeChecker WithTypeInfoMap(const TypeInfoMap& typeinfo) const {
    assert(!belowCompUnit_);
    return TypeChecker(fs_, errors_, typeset_, typeinfo);
  }

  TypeChecker InsideCompUnit(sptr<const ast::QualifiedName> package) const {
    assert(!belowCompUnit_);
    return TypeChecker(fs_, errors_, typeset_, typeinfo_, true, package);
  }

  TypeChecker InsideTypeDecl(ast::TypeId curtype) const {
    assert(belowCompUnit_);
    assert(!belowTypeDecl_);
    return TypeChecker(fs_, errors_, typeset_, typeinfo_, true, package_, true, curtype);
  }

  TypeChecker InsideMethodDecl(ast::TypeId curMethRet) const {
    assert(belowTypeDecl_);
    assert(!belowMethodDecl_);
    return TypeChecker(fs_, errors_, typeset_, typeinfo_, true, package_, true, curtype_, true, curMethRet);
  }

  REWRITE_DECL(ArrayIndexExpr, Expr, expr, exprptr);
  REWRITE_DECL(BinExpr, Expr, expr, exprptr);
  REWRITE_DECL(BoolLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(CastExpr, Expr, expr, exprptr);
  REWRITE_DECL(CharLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(InstanceOfExpr, Expr, expr, exprptr);
  REWRITE_DECL(IntLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(NewArrayExpr, Expr, expr, exprptr);
  REWRITE_DECL(NullLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(ParenExpr, Expr, expr, exprptr);
  REWRITE_DECL(StringLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(ThisExpr, Expr, expr, exprptr);
  REWRITE_DECL(UnaryExpr, Expr, expr, exprptr);

  REWRITE_DECL(ForStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(IfStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(LocalDeclStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(ReturnStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(WhileStmt, Stmt, stmt, stmtptr);

  REWRITE_DECL(FieldDecl, MemberDecl, decl, declptr);
  REWRITE_DECL(MethodDecl, MemberDecl, decl, declptr);

  REWRITE_DECL(TypeDecl, TypeDecl, args, argsptr);

  REWRITE_DECL(CompUnit, CompUnit, args, argsptr);

 private:
  FRIEND_TEST(TypeCheckerUtilTest, IsCastablePrimitives);

  TypeChecker(const base::FileSet* fs, base::ErrorList* errors,
              const TypeSet& typeset, const TypeInfoMap& typeinfo,
              bool belowCompUnit = false, sptr<const ast::QualifiedName> package = nullptr,
              bool belowTypeDecl = false, ast::TypeId curtype = ast::TypeId::kUnassigned,
              bool belowMethodDecl = false, ast::TypeId curMethRet = ast::TypeId::kUnassigned)
      : fs_(fs), errors_(errors), typeset_(typeset), typeinfo_(typeinfo),
        belowCompUnit_(belowCompUnit), package_(package),
        belowTypeDecl_(belowTypeDecl), curtype_(curtype),
        belowMethodDecl_(belowMethodDecl), curMethRet_(curMethRet) {}

  sptr<const ast::Type> MustResolveType(sptr<const ast::Type> type);
  ast::TypeId JavaLangType(const string& name) const;

  bool IsNumeric(ast::TypeId tid) const;
  bool IsPrimitive(ast::TypeId tid) const;
  bool IsReference(ast::TypeId tid) const;
  bool IsPrimitiveWidening(ast::TypeId lhs, ast::TypeId rhs) const;
  bool IsPrimitiveNarrowing(ast::TypeId lhs, ast::TypeId rhs) const;
  bool IsReferenceWidening(ast::TypeId lhs, ast::TypeId rhs) const;
  bool IsAssignable(ast::TypeId lhs, ast::TypeId rhs) const;
  bool IsComparable(ast::TypeId lhs, ast::TypeId rhs) const;
  bool IsCastable(ast::TypeId lhs, ast::TypeId rhs) const;

  base::Error* MakeTypeMismatchError(ast::TypeId expected, ast::TypeId got, base::PosRange pos);
  base::Error* MakeIndexNonArrayError(base::PosRange pos);
  base::Error* MakeIncompatibleCastError(ast::TypeId lhs, ast::TypeId rhs, base::PosRange pos);
  base::Error* MakeInstanceOfPrimitiveError(base::PosRange pos);
  base::Error* MakeIncompatibleInstanceOfError(ast::TypeId lhs, ast::TypeId rhs, base::PosRange pos);
  base::Error* MakeNoStringError(base::PosRange pos);
  base::Error* MakeUnaryNonNumericError(ast::TypeId rhs, base::PosRange pos);
  base::Error* MakeUnaryNonBoolError(ast::TypeId rhs, base::PosRange pos);
  base::Error* MakeUnassignableError(ast::TypeId lhs, ast::TypeId rhs, base::PosRange pos);
  base::Error* MakeInvalidReturnError(ast::TypeId ret, ast::TypeId expr, base::PosRange pos);
  base::Error* MakeIncomparableTypeError(ast::TypeId lhs, ast::TypeId rhs, base::PosRange pos);

  const base::FileSet* fs_;
  base::ErrorList* errors_;

  const TypeSet& typeset_;
  const TypeInfoMap& typeinfo_;

  const bool belowCompUnit_ = false;
  const sptr<const ast::QualifiedName> package_; // Only populated if below CompUnit and the CompUnit has a package statement.

  const bool belowTypeDecl_ = false;
  const ast::TypeId curtype_; // Only populated if below TypeDecl.

  const bool belowMethodDecl_ = false;
  const ast::TypeId curMethRet_; // Only populated if below MethodDecl.
};

}  // namespace types

#endif
