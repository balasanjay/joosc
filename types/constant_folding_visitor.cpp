#include "types/constant_folding_visitor.h"

#include "lexer/lexer.h"
#include "ast/extent.h"

using ast::Expr;
using ast::FoldedConstantExpr;

namespace types {

/*
REWRITE_DEFN(ConstantFoldingVisitor, StringLitExpr, Expr, expr, exprptr) {
}

REWRITE_DEFN(ConstantFoldingVisitor, CharLitExpr, Expr, expr, exprptr) {
}
*/

REWRITE_DEFN(ConstantFoldingVisitor, IntLitExpr, Expr, , exprptr) {
  return make_shared<FoldedConstantExpr>(exprptr, exprptr);
}

REWRITE_DEFN(ConstantFoldingVisitor, BoolLitExpr, Expr, , exprptr) {
  return make_shared<FoldedConstantExpr>(exprptr, exprptr);
}

REWRITE_DEFN(ConstantFoldingVisitor, BinExpr, Expr, expr, exprptr) {
  // TODO: Chars and Strings.

  sptr<const Expr> lhs = Rewrite(expr.LhsPtr());
  sptr<const Expr> rhs = Rewrite(expr.RhsPtr());
  auto lhs_const = dynamic_cast<const FoldedConstantExpr*>(lhs.get());
  auto rhs_const = dynamic_cast<const FoldedConstantExpr*>(rhs.get());
  if (lhs_const == nullptr || rhs_const == nullptr) {
    return exprptr;
  }

  if (lexer::IsBoolOp(expr.Op().type)) {
    bool lhs_value =
      dynamic_cast<const ast::BoolLitExpr*>(
        lhs_const->ConstantPtr().get()
        )->GetToken().type == lexer::K_TRUE;
    bool rhs_value =
      dynamic_cast<const ast::BoolLitExpr*>(
        rhs_const->ConstantPtr().get()
        )->GetToken().type == lexer::K_TRUE;
    bool result = false;
    switch (expr.Op().type) {
      case lexer::OR: result = lhs_value || rhs_value; break;
      case lexer::AND: result = lhs_value && rhs_value; break;
      default: break;
    }

    auto new_bool_expr = make_shared<ast::BoolLitExpr>(lexer::Token(result ? lexer::K_TRUE : lexer::K_FALSE, ExtentOf(exprptr)));
    return make_shared<FoldedConstantExpr>(new_bool_expr, exprptr);

  } else if (IsNumericOp(expr.Op().type)) {
  }

  return exprptr;
}

REWRITE_DEFN(ConstantFoldingVisitor, UnaryExpr, Expr, expr, exprptr) {
  if (expr.Op().type != lexer::SUB) {
    return exprptr;
  }

  auto rhs = Rewrite(expr.RhsPtr());
  auto rhs_const = dynamic_cast<const FoldedConstantExpr*>(rhs.get());
  if (rhs_const == nullptr) {
    return exprptr;
  }

  auto int_lit = dynamic_cast<const ast::IntLitExpr*>(rhs_const->ConstantPtr().get());
  if (int_lit == nullptr) {
    return exprptr;
  }

  i64 new_int_value = -int_lit->Value();
  auto new_int_lit = make_shared<ast::IntLitExpr>(
      lexer::Token(lexer::INTEGER, ExtentOf(exprptr)), new_int_value, expr.GetTypeId());
  return make_shared<FoldedConstantExpr>(new_int_lit, exprptr);
}

} // namespace types

