#include "base/macros.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "weeder/type_visitor.h"

using base::Error;
using base::ErrorList;
using base::FileSet;
using base::Pos;
using lexer::ASSG;
using lexer::K_VOID;
using lexer::Token;
using parser::ArrayType;
using parser::BinExpr;
using parser::BlockStmt;
using parser::CallExpr;
using parser::CastExpr;
using parser::EmptyStmt;
using parser::Expr;
using parser::ExprStmt;
using parser::FieldDecl;
using parser::LocalDeclStmt;
using parser::NameExpr;
using parser::NewArrayExpr;
using parser::NewClassExpr;
using parser::Param;
using parser::PrimitiveType;
using parser::ReferenceType;
using parser::Stmt;
using parser::Type;

namespace weeder {
namespace {

// Accepts assignment expressions, CallExpr, NewClassExpr.
bool IsTopLevelExpr(const Expr* expr) {
  if (expr == nullptr) {
    return true;
  }
  if (IS_CONST_PTR(BinExpr, expr)) {
    const BinExpr* binExpr = dynamic_cast<const BinExpr*>(expr);
    return binExpr->Op().type == ASSG;
  }
  return IS_CONST_PTR(CallExpr, expr) || IS_CONST_PTR(NewClassExpr, expr);
}

// Accepts EmptyStmt and what IsTopLevelExpr(Expr*) accepts
// (assignment expressions, CallExpr, NewClassExpr).
bool IsTopLevelExpr(const Stmt* stmt) {
  if (IS_CONST_PTR(ExprStmt, stmt)) {
    const Expr* expr = dynamic_cast<const ExprStmt*>(stmt)->GetExpr();
    return IsTopLevelExpr(expr);
  }
  return IS_CONST_PTR(EmptyStmt, stmt);
}

// Rejects ExprStmts that are not top level expressions.
// Accepts all other Stmts.
bool IsTopLevelStmt(const Stmt* stmt) {
  // Only fail if it is an expr that is not top level.
  if (IS_CONST_PTR(ExprStmt, stmt)) {
    return IsTopLevelExpr(stmt);
  }
  return true;
}

Error* MakeInvalidVoidTypeError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "InvalidVoidTypeError",
      "'void' is only valid as the return type of a method.");
}

Error* MakeNewNonReferenceTypeError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "NewNonReferenceTypeError",
      "Can only instantiate non-array reference types.");
}

Error* MakeInvalidInstanceOfType(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "InvalidInstanceOfTypeError",
      "Right-hand-side of 'instanceof' must be a reference type or an array.");
}

Error* MakeInvalidTopLevelStatement(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "InvalidTopLevelStatement",
      "A top level statement can only be an assignment, a method call, or a class instantiation.");
}
} // namespace

bool HasVoid(const Type* type, Token* out) {
  const Type* cur = type;

  while (true) {
    // Reference types.
    if (IS_CONST_PTR(ReferenceType, cur)) {
      return false;
    }

    // Array types.
    if (IS_CONST_PTR(ArrayType, cur)) {
      const ArrayType* array = dynamic_cast<const ArrayType*>(cur);
      cur = array->ElemType();
      continue;
    }

    // Primitive types.
    assert(IS_CONST_PTR(PrimitiveType, cur));
    const PrimitiveType* prim = dynamic_cast<const PrimitiveType*>(cur);
    if (prim->GetToken().type == K_VOID) {
      *out = prim->GetToken();
      return true;
    }
    return false;
  }
}

REC_VISIT_DEFN(TypeVisitor, CastExpr, expr) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(expr->GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
  }
  return true;
}

REC_VISIT_DEFN(TypeVisitor, InstanceOfExpr, expr) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(expr->GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
  } else if (IS_CONST_PTR(PrimitiveType, expr->GetType())) {
    errors_->Append(MakeInvalidInstanceOfType(fs_, expr->InstanceOf()));
  }
  return true;
}

REC_VISIT_DEFN(TypeVisitor, NewClassExpr, expr) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(expr->GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
  } else if (!IS_CONST_PTR(ReferenceType, expr->GetType())) {
    errors_->Append(MakeNewNonReferenceTypeError(fs_, expr->NewToken()));
  }
  return true;
}

REC_VISIT_DEFN(TypeVisitor, NewArrayExpr, expr) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(expr->GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
  }
  return true;
}

REC_VISIT_DEFN(TypeVisitor, LocalDeclStmt, stmt) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(stmt->GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
  }
  return true;
}

REC_VISIT_DEFN(TypeVisitor, FieldDecl, stmt) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(stmt->GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
  }
  return true;
}

REC_VISIT_DEFN(TypeVisitor, Param, param) {
  Token voidTok(K_VOID, Pos(-1, -1));
  if (HasVoid(param->GetType(), &voidTok)) {
    errors_->Append(MakeInvalidVoidTypeError(fs_, voidTok));
  }
  return true;
}

REC_VISIT_DEFN(TypeVisitor, ForStmt, stmt) {
  if (!IS_CONST_PTR(LocalDeclStmt, stmt->Init()) &&
      !IsTopLevelExpr(stmt->Init())) {
    // TODO: Error.
    Token tok(K_VOID, Pos(0, 1));
    errors_->Append(MakeInvalidTopLevelStatement(fs_, tok));
  }
  if (!IsTopLevelExpr(stmt->Update())) {
    // TODO: Error.
    Token tok(K_VOID, Pos(0, 1));
    errors_->Append(MakeInvalidTopLevelStatement(fs_, tok));
  }
  return true;
}

REC_VISIT_DEFN(TypeVisitor, BlockStmt, block) {
  for (int i = 0; i < block->Stmts().Size(); ++i) {
    if (!IsTopLevelStmt(block->Stmts().At(i))) {
      // TODO: Error.
      Token tok(K_VOID, Pos(0, 1));
      errors_->Append(MakeInvalidTopLevelStatement(fs_, tok));
    }
  }
  return true;
}

} // namespace weeder
