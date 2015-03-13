#include "types/constant_folding_visitor.h"

#include "lexer/lexer.h"
#include "ast/extent.h"

using ast::Expr;
using ast::ConstExpr;

namespace types {

namespace {



} // namespace

REWRITE_DEFN(ConstantFoldingVisitor, ConstExpr, Expr, , exprptr) {
  // Simply return the folded constant expr so that this pass is idempotent.
  return exprptr;
}

// TODO: Rewrite StringLitExpr.
// TODO: Rewrite CharLitExpr.
// TODO: Casting between primitive types.

REWRITE_DEFN(ConstantFoldingVisitor, IntLitExpr, Expr, , exprptr) {
  return make_shared<ConstExpr>(exprptr, exprptr);
}

REWRITE_DEFN(ConstantFoldingVisitor, BoolLitExpr, Expr, , exprptr) {
  return make_shared<ConstExpr>(exprptr, exprptr);
}

REWRITE_DEFN(ConstantFoldingVisitor, BinExpr, Expr, expr, exprptr) {
  sptr<const Expr> lhs = Rewrite(expr.LhsPtr());
  sptr<const Expr> rhs = Rewrite(expr.RhsPtr());
  auto lhs_const = dynamic_cast<const ConstExpr*>(lhs.get());
  auto rhs_const = dynamic_cast<const ConstExpr*>(rhs.get());
  if (lhs_const == nullptr || rhs_const == nullptr) {
    return exprptr;
  }

  auto original_stripped = make_shared<ast::BinExpr>(
      lhs_const->OriginalPtr(), expr.Op(), rhs_const->OriginalPtr(), expr.GetTypeId());

  if (lexer::IsBoolOp(expr.Op().type)) {
    auto lhs = dynamic_cast<const ast::BoolLitExpr*>(
        lhs_const->ConstantPtr().get());
    auto rhs = dynamic_cast<const ast::BoolLitExpr*>(
        rhs_const->ConstantPtr().get());
    CHECK(lhs != nullptr && rhs != nullptr);

    bool lhs_value = lhs->GetToken().type == lexer::K_TRUE;
    bool rhs_value = rhs->GetToken().type == lexer::K_TRUE;
    bool result = false;

    switch (expr.Op().type) {
      case lexer::OR: result = lhs_value || rhs_value; break;
      case lexer::AND: result = lhs_value && rhs_value; break;
      default: break;
    }

    auto new_bool_expr = make_shared<ast::BoolLitExpr>(
        lexer::Token(result ? lexer::K_TRUE : lexer::K_FALSE, ExtentOf(exprptr)), ast::TypeId::kBool);
    return make_shared<ConstExpr>(new_bool_expr, original_stripped);
  }

  if (IsNumericOp(expr.Op().type)) {
    // TODO: Chars, Strings,
    auto lhs = dynamic_cast<const ast::IntLitExpr*>(
        lhs_const->ConstantPtr().get());
    auto rhs = dynamic_cast<const ast::IntLitExpr*>(
        rhs_const->ConstantPtr().get());

    CHECK(lhs != nullptr && rhs != nullptr);

    i64 lhs_value = lhs->Value();
    i64 rhs_value = rhs->Value();
    i64 result = 0;

    // TODO: Perform correct overflow and masking depending on size of int/short/byte.
    switch (expr.Op().type) {
      case lexer::ADD: result = lhs_value + rhs_value; break;
      case lexer::SUB: result = lhs_value - rhs_value; break;
      case lexer::MUL: result = lhs_value * rhs_value; break;
      case lexer::DIV:
        // If dividing by 0, don't fold constants.
        if (rhs_value == 0) {
          return exprptr;
        }
        result = lhs_value / rhs_value;
        break;
      case lexer::MOD:
        // If dividing by 0, don't fold constants.
        if (rhs_value == 0) {
          return exprptr;
        }
        result = lhs_value % rhs_value;
        break;
      default: break;
    }

    auto new_int_expr = make_shared<ast::IntLitExpr>(
        lexer::Token(lexer::INTEGER, ExtentOf(exprptr)), result, ast::TypeId::kInt);
    return make_shared<ConstExpr>(new_int_expr, original_stripped);

  }

  if (IsRelationalOp(expr.Op().type) || IsEqualityOp(expr.Op().type)) {
    // TODO: Relational and equality checks for other types.
    auto lhs = dynamic_cast<const ast::IntLitExpr*>(
        lhs_const->ConstantPtr().get());
    auto rhs = dynamic_cast<const ast::IntLitExpr*>(
        rhs_const->ConstantPtr().get());

    if (lhs != nullptr && rhs != nullptr) {
      i64 lhs_value = lhs->Value();
      i64 rhs_value = rhs->Value();
      bool result = 0;
      switch (expr.Op().type) {
        case lexer::LE: result = lhs_value <= rhs_value; break;
        case lexer::GE: result = lhs_value >= rhs_value; break;
        case lexer::LT: result = lhs_value < rhs_value; break;
        case lexer::GT: result = lhs_value > rhs_value; break;

        case lexer::EQ: result = lhs_value == rhs_value; break;
        case lexer::NEQ: result = lhs_value != rhs_value; break;

        default: break;
      }

      auto new_bool_expr = make_shared<ast::BoolLitExpr>(
          lexer::Token(result ? lexer::K_TRUE : lexer::K_FALSE, ExtentOf(exprptr)), ast::TypeId::kBool);
      return make_shared<ConstExpr>(new_bool_expr, original_stripped);
    }
  }

  return exprptr;
}

REWRITE_DEFN(ConstantFoldingVisitor, UnaryExpr, Expr, expr, exprptr) {
  auto rhs = Rewrite(expr.RhsPtr());
  auto rhs_const = dynamic_cast<const ConstExpr*>(rhs.get());
  if (rhs_const == nullptr) {
    return exprptr;
  }

  if (expr.Op().type == lexer::SUB) {
    auto int_lit = dynamic_cast<const ast::IntLitExpr*>(rhs_const->ConstantPtr().get());
    CHECK(int_lit != nullptr);

    i64 new_int_value = -int_lit->Value();
    auto new_int_lit = make_shared<ast::IntLitExpr>(
        lexer::Token(lexer::INTEGER, ExtentOf(exprptr)), new_int_value, ast::TypeId::kInt);
    return make_shared<ConstExpr>(new_int_lit, exprptr);
  }

  if (expr.Op().type == lexer::NOT) {
    auto bool_lit = dynamic_cast<const ast::BoolLitExpr*>(rhs_const->ConstantPtr().get());
    CHECK(bool_lit != nullptr);

    bool new_bool_value = !(bool_lit->GetToken().type == lexer::K_TRUE);
    auto new_bool_lit = make_shared<ast::BoolLitExpr>(
        lexer::Token(new_bool_value ? lexer::K_TRUE : lexer::K_FALSE, ExtentOf(exprptr)), ast::TypeId::kBool);
    return make_shared<ConstExpr>(new_bool_lit, exprptr);
  }

  return exprptr;
}

} // namespace types

