#include "weeder/type_visitor.h"

#include "ast/ast.h"
#include "ast/extent.h"
#include "base/macros.h"
#include "lexer/lexer.h"

using ast::ArrayType;
using ast::BinExpr;
using ast::BlockStmt;
using ast::CallExpr;
using ast::CastExpr;
using ast::EmptyStmt;
using ast::Expr;
using ast::ExprStmt;
using ast::ExtentOf;
using ast::FieldDecl;
using ast::LocalDeclStmt;
using ast::NameExpr;
using ast::NewArrayExpr;
using ast::NewClassExpr;
using ast::Param;
using ast::PrimitiveType;
using ast::ReferenceType;
using ast::Stmt;
using ast::Type;
using ast::VisitResult;
using base::Error;
using base::ErrorList;
using base::FileSet;
using base::Pos;
using base::PosRange;
using lexer::ASSG;
using lexer::K_VOID;
using lexer::Token;

namespace weeder {
namespace {

// Accepts assignment expressions, CallExpr, NewClassExpr.
bool IsTopLevelExpr(const Expr* expr) {
  if (IS_CONST_PTR(BinExpr, expr)) {
    const BinExpr* binExpr = dynamic_cast<const BinExpr*>(expr);
    return binExpr->Op().type == ASSG;
  }
  return IS_CONST_PTR(CallExpr, expr) || IS_CONST_PTR(NewClassExpr, expr);
}

Error* MakeInvalidVoidTypeError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "InvalidVoidTypeError",
      "'void' is only valid as the return type of a method.");
}

Error* MakeNewNonReferenceTypeError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "NewNonReferenceTypeError",
      "Can only instantiate non-array reference types.");
}

Error* MakeInvalidInstanceOfType(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "InvalidInstanceOfTypeError",
      "Right-hand-side of 'instanceof' must be a reference type or an array.");
}

Error* MakeInvalidTopLevelStatement(const FileSet* fs, PosRange pos) {
  return MakeSimplePosRangeError(fs, pos, "InvalidTopLevelStatement",
                                 "A top level statement can only be an "
                                 "assignment, a method call, or a class "
                                 "instantiation.");
}
}  // namespace

bool HasVoid(const Type& type, Token* out) {
  const Type* cur = &type;

  while (true) {
    // Reference types.
    if (IS_CONST_PTR(ReferenceType, cur)) {
      return false;
    }

    // Array types.
    if (IS_CONST_PTR(ArrayType, cur)) {
      const Type& array = dynamic_cast<const ArrayType*>(cur)->ElemType();
      cur = &array;
      continue;
    }

    // Primitive types.
    CHECK(IS_CONST_PTR(PrimitiveType, cur));
    const PrimitiveType* prim = dynamic_cast<const PrimitiveType*>(cur);
    if (prim->GetToken().type == K_VOID) {
      *out = prim->GetToken();
      return true;
    }
    return false;
  }
}

VISIT_DEFN(TypeVisitor, CastExpr, expr,) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(expr.GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN(TypeVisitor, InstanceOfExpr, expr,) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(expr.GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
  } else if (IS_CONST_REF(PrimitiveType, expr.GetType())) {
    errors_->Append(MakeInvalidInstanceOfType(fs_, expr.InstanceOf()));
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN(TypeVisitor, NewClassExpr, expr,) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(expr.GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
    return VisitResult::RECURSE_PRUNE;
  } else if (!IS_CONST_REF(ReferenceType, expr.GetType())) {
    errors_->Append(MakeNewNonReferenceTypeError(fs_, expr.NewToken()));
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN(TypeVisitor, NewArrayExpr, expr,) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(expr.GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN(TypeVisitor, LocalDeclStmt, stmt,) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(stmt.GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN(TypeVisitor, FieldDecl, stmt,) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(stmt.GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN(TypeVisitor, Param, param,) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(param.GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN(TypeVisitor, ForStmt, stmt,) {
  // Check only update expr.
  sptr<const Expr> updatePtr = stmt.UpdatePtr();
  if (updatePtr != nullptr && !IsTopLevelExpr(updatePtr.get())) {
    errors_->Append(MakeInvalidTopLevelStatement(fs_, ExtentOf(stmt.UpdatePtr())));
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

VISIT_DEFN(TypeVisitor, ExprStmt, stmt, stmtptr) {
  if (!IsTopLevelExpr(stmt.GetExprPtr().get())) {
    errors_->Append(MakeInvalidTopLevelStatement(fs_, ExtentOf(stmtptr)));
    return VisitResult::RECURSE_PRUNE;
  }
  return VisitResult::RECURSE;
}

}  // namespace weeder
