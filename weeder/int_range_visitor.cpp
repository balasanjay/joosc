#include "weeder/int_range_visitor.h"

#include "ast/ast.h"
#include "base/macros.h"
#include "parser/parser.h"

using ast::IntLitExpr;
using ast::VisitResult;
using base::Error;
using base::FileSet;
using lexer::SUB;
using lexer::Token;

namespace weeder {

namespace {

Error* MakeInvalidIntRangeError(Token token) {
  return MakeSimplePosRangeError(
      token.pos, "InvalidIntRangeError",
      "Ints must be between -2^-31 and 2^31 - 1 inclusive.");
}

pair<i64, bool> ConvertInt(const string& str_val, Token token, bool is_negated, base::ErrorList* errors) {
  const i64 INT_32_MIN = -(1L << 31L);
  const i64 INT_32_MAX = (1L << 31L) - 1L;
  i64 intVal = 0;

  stringstream ss(str_val);
  ss >> intVal;

  if (is_negated) {
    intVal *= -1;
  }

  if (intVal < INT_32_MIN || intVal > INT_32_MAX || !ss) {
    errors->Append(MakeInvalidIntRangeError(token));
    return {0, false};
  }
  return {intVal, true};
}


}  // namespace

REWRITE_DEFN(IntRangeVisitor, IntLitExpr, Expr, expr,) {
  Token token = expr.GetToken();
  const string& str_val = parser::TokenString(fs_->Get(token.pos.fileid), token);
  auto int_result = ConvertInt(str_val, token, false, errors_);

  if (!int_result.second) {
    return nullptr;
  }

  return make_shared<IntLitExpr>(token, int_result.first);
}

REWRITE_DEFN(IntRangeVisitor, UnaryExpr, Expr, expr,) {
  if (expr.Op().type != SUB || !IS_CONST_REF(IntLitExpr, expr.Rhs())) {
    auto rhs = Rewrite(expr.RhsPtr());
    if (rhs == nullptr) {
      return nullptr;
    }
    return make_shared<ast::UnaryExpr>(expr.Op(), rhs);
  }

  Token token = dynamic_cast<const IntLitExpr&>(expr.Rhs()).GetToken();
  const string& str_val = parser::TokenString(fs_->Get(token.pos.fileid), token);
  auto int_result = ConvertInt(str_val, token, true, errors_);
  if (!int_result.second) {
    return nullptr;
  }

  // We know we're at a base-case, an in-range negative int-literal, so we can
  // skip the recursion. Otherwise, the int-literal might get caught as
  // out-of-range for a positive int-literal.
  return make_shared<IntLitExpr>(expr.Op(), int_result.first);
}

}  // namespace weeder
