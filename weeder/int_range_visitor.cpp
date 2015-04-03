#include "weeder/int_range_visitor.h"

#include <limits>

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

pair<i32, bool> ConvertInt(const string& str_val, Token token, bool is_negated, base::ErrorList* errors) {
  i64 intVal = 0;

  stringstream ss(str_val);
  ss >> intVal;

  if (is_negated) {
    intVal *= -1;
  }

  if (intVal < std::numeric_limits<i32>::min() || intVal > std::numeric_limits<i32>::max() || !ss) {
    errors->Append(MakeInvalidIntRangeError(token));
    return {0, false};
  }
  return {(i32)intVal, true};
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
  Token result = expr.Op();
  result.type = lexer::INTEGER;
  result.pos.end = token.pos.end;
  return make_shared<IntLitExpr>(result, int_result.first);
}

}  // namespace weeder
