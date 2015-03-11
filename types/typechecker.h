#ifndef TYPES_TYPECHECKER_H
#define TYPES_TYPECHECKER_H

#include "third_party/gtest/gtest.h"
#include "ast/visitor.h"
#include "base/errorlist.h"
#include "types/type_info_map.h"
#include "types/typeset.h"
#include "types/symbol_table.h"

namespace types {

class TypeChecker final : public ast::Visitor {
 public:
   TypeChecker(base::ErrorList* errors) : TypeChecker(errors, TypeSet::Empty(), TypeInfoMap::Empty()) {}

  TypeChecker WithTypeSet(const TypeSet& typeset) const {
    CHECK(!belowCompUnit_);
    return TypeChecker(errors_, typeset, typeinfo_);
  }

  TypeChecker WithTypeInfoMap(const TypeInfoMap& typeinfo) const {
    CHECK(!belowCompUnit_);
    return TypeChecker(errors_, typeset_, typeinfo);
  }

  TypeChecker InsideCompUnit(sptr<const ast::QualifiedName> package) const {
    CHECK(!belowCompUnit_);
    return TypeChecker(errors_, typeset_, typeinfo_, true, package);
  }

  TypeChecker InsideTypeDecl(ast::TypeId curtype, const TypeSet& typeset) const {
    CHECK(belowCompUnit_);
    CHECK(!belowTypeDecl_);
    return TypeChecker(errors_, typeset, typeinfo_, true, package_, true, curtype);
  }

  TypeChecker InsideMemberDecl(bool is_static, ast::TypeId cur_member_type, const ast::ParamList& params) const {
    vector<VariableInfo> paramInfos;
    for (int i = 0; i < params.Params().Size(); ++i) {
      sptr<const ast::Param> param = params.Params().At(i);
      paramInfos.push_back(VariableInfo(
        param->GetType().GetTypeId(),
        param->Name(),
        param->NameToken().pos));
    }

    return InsideMemberDecl(is_static, cur_member_type, paramInfos);
  }

  TypeChecker InsideMemberDecl(
      bool is_static,
      ast::TypeId cur_member_type = ast::TypeId::kError,
      const vector<VariableInfo>& paramInfos = {}) const {
    CHECK(belowTypeDecl_);
    CHECK(!belowMemberDecl_);

    // Construct initial symbol table with params for this method.
    return TypeChecker(
        errors_, typeset_, typeinfo_,
        true, package_,
        true, curtype_,
        true, is_static, cur_member_type, SymbolTable(paramInfos, errors_));
  }

  REWRITE_DECL(ArrayIndexExpr, Expr, expr, exprptr);
  REWRITE_DECL(BinExpr, Expr, expr, exprptr);
  REWRITE_DECL(BoolLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(CallExpr, Expr, expr, exprptr);
  REWRITE_DECL(CastExpr, Expr, expr, exprptr);
  REWRITE_DECL(CharLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(InstanceOfExpr, Expr, expr, exprptr);
  REWRITE_DECL(IntLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(NameExpr, Expr, expr,);
  REWRITE_DECL(NewArrayExpr, Expr, expr, exprptr);
  REWRITE_DECL(NewClassExpr, Expr, expr, exprptr);
  REWRITE_DECL(NullLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(FieldDerefExpr, Expr, expr,);
  REWRITE_DECL(ParenExpr, Expr, expr, exprptr);
  REWRITE_DECL(StringLitExpr, Expr, expr, exprptr);
  REWRITE_DECL(ThisExpr, Expr, expr, exprptr);
  REWRITE_DECL(UnaryExpr, Expr, expr, exprptr);

  REWRITE_DECL(ForStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(IfStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(LocalDeclStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(ReturnStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(WhileStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(BlockStmt, Stmt, stmt, stmtptr);
  REWRITE_DECL(Param, Param, param, paramptr);

  REWRITE_DECL(FieldDecl, MemberDecl, decl, declptr);
  REWRITE_DECL(MethodDecl, MemberDecl, decl, declptr);

  REWRITE_DECL(TypeDecl, TypeDecl, args, argsptr);

  REWRITE_DECL(CompUnit, CompUnit, args, argsptr);

 private:
  FRIEND_TEST(TypeCheckerHierarchyTest, IsCastablePrimitives);

  TypeChecker(base::ErrorList* errors,
              const TypeSet& typeset, const TypeInfoMap& typeinfo,
              bool belowCompUnit = false, sptr<const ast::QualifiedName> package = nullptr,
              bool belowTypeDecl = false, ast::TypeId curtype = ast::TypeId::kUnassigned,
              bool belowMemberDecl = false, bool belowStaticMember = false, ast::TypeId curMethRet = ast::TypeId::kUnassigned,
              SymbolTable symbol_table = SymbolTable::Empty())
      : errors_(errors), typeset_(typeset), typeinfo_(typeinfo),
        belowCompUnit_(belowCompUnit), package_(package),
        belowTypeDecl_(belowTypeDecl), curtype_(curtype),
        belowMemberDecl_(belowMemberDecl), belowStaticMember_(belowStaticMember), curMemberType_(curMethRet),
        symbol_table_(symbol_table) {}

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
  base::Error* MakeThisInStaticMemberError(base::PosRange this_pos);

  base::ErrorList* errors_;

  const TypeSet& typeset_;
  const TypeInfoMap& typeinfo_;

  const bool belowCompUnit_ = false;
  const sptr<const ast::QualifiedName> package_; // Only populated if below CompUnit and the CompUnit has a package statement.

  const bool belowTypeDecl_ = false;
  const ast::TypeId curtype_; // Only populated if below TypeDecl.

  const bool belowMemberDecl_ = false;
  const bool belowStaticMember_ = false;
  const ast::TypeId curMemberType_; // Only populated if below MethodDecl.
  SymbolTable symbol_table_; // Empty unless below MethodDecl.
};

}  // namespace types

#endif
