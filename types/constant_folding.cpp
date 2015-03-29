#include "types/constant_folding.h"

#include "lexer/lexer.h"
#include "ast/extent.h"
#include "ast/visitor.h"
#include "types/type_info_map.h"
#include "types/typechecker.h"

using ast::Expr;
using ast::ConstExpr;

namespace types {

StringId kFirstStringId = 0;

class ConstantFoldingVisitor final : public ast::Visitor {
public:
  ConstantFoldingVisitor(ConstStringMap* strings, ast::TypeId string_type): strings_(*strings), string_type_(string_type) {
  }

  void AddString(const jstring& s) {
    if (strings_.count(s) == 0) {
      strings_.insert({s, next_string_id_});
      ++next_string_id_;
    }
  }

  // Convert either a ConstExpr wrapping one of the
  // literals, or just a literal, into its string
  // representation.
  jstring Stringify(sptr<const Expr> expr) {
    // TODO: Null literals.
    sptr<const Expr> inside_const = expr;
    auto const_expr = dynamic_cast<const ast::ConstExpr*>(expr.get());
    if (const_expr != nullptr) {
      inside_const = const_expr->ConstantPtr();
    }

    if (inside_const->GetTypeId() == string_type_) {
      auto inner_const_str = dynamic_cast<const ast::StringLitExpr*>(inside_const.get());
      CHECK(inner_const_str != nullptr);
      return inner_const_str->Str();
    }

    if (inside_const->GetTypeId() == ast::TypeId::kChar) {
      auto inner_const_char = dynamic_cast<const ast::CharLitExpr*>(inside_const.get());
      CHECK(inner_const_char != nullptr);

      return jstring(1, inner_const_char->Char());
    }

    if (TypeChecker::IsNumeric(inside_const->GetTypeId())) {
      auto inner_const_int = dynamic_cast<const ast::IntLitExpr*>(inside_const.get());
      CHECK(inner_const_int != nullptr);

      i64 value = inner_const_int->Value();
      stringstream ss;
      ss << value;
      string s = ss.str();
      jstring js;
      js.insert(js.begin(), s.begin(), s.end());
      return js;
    }

    CHECK(inside_const->GetTypeId() == ast::TypeId::kBool);

    auto inner_const_bool = dynamic_cast<const ast::BoolLitExpr*>(inside_const.get());
    CHECK(inner_const_bool != nullptr);
    string s = inner_const_bool->GetToken().type == lexer::K_TRUE ? "true" : "false";
    jstring js;
    js.insert(js.begin(), s.begin(), s.end());
    return js;
  }

  // Get integer value from an int or character literal.
  i64 GetIntValue(sptr<const Expr> expr) {
    if (expr->GetTypeId() == ast::TypeId::kChar) {
      auto char_expr = dynamic_cast<const ast::CharLitExpr*>(expr.get());
      CHECK(char_expr != nullptr);
      return (u64)char_expr->Char();
    }

    auto int_expr = dynamic_cast<const ast::IntLitExpr*>(expr.get());
    CHECK(int_expr != nullptr);
    return int_expr->Value();
  }

  REWRITE_DECL(ConstExpr, Expr, , exprptr) {
    // Simply return the folded constant expr so that this pass is idempotent.
    return exprptr;
  }

  REWRITE_DECL(IntLitExpr, Expr, , exprptr) {
    return make_shared<ConstExpr>(exprptr, exprptr);
  }

  REWRITE_DECL(CharLitExpr, Expr, , exprptr) {
    return make_shared<ConstExpr>(exprptr, exprptr);
  }

  REWRITE_DECL(BoolLitExpr, Expr, , exprptr) {
    return make_shared<ConstExpr>(exprptr, exprptr);
  }

  REWRITE_DECL(StringLitExpr, Expr, expr, exprptr) {
    AddString(expr.Str());
    return make_shared<ConstExpr>(exprptr, exprptr);
  }

  REWRITE_DECL(BinExpr, Expr, expr, exprptr) {
    sptr<const Expr> lhs = Rewrite(expr.LhsPtr());
    sptr<const Expr> rhs = Rewrite(expr.RhsPtr());
    auto lhs_const = dynamic_cast<const ConstExpr*>(lhs.get());
    auto rhs_const = dynamic_cast<const ConstExpr*>(rhs.get());
    if (lhs_const == nullptr || rhs_const == nullptr) {
      return exprptr;
    }

    ast::TypeId lhs_type = lhs_const->Constant().GetTypeId();
    ast::TypeId rhs_type = rhs_const->Constant().GetTypeId();

    if (lexer::IsBoolOp(expr.Op().type)) {
      auto lhs = dynamic_cast<const ast::BoolLitExpr*>(
          lhs_const->ConstantPtr().get());
      auto rhs = dynamic_cast<const ast::BoolLitExpr*>(
          rhs_const->ConstantPtr().get());
      CHECK(lhs != nullptr && rhs != nullptr);

      bool lhs_value = (lhs->GetToken().type == lexer::K_TRUE);
      bool rhs_value = (rhs->GetToken().type == lexer::K_TRUE);
      bool result = false;

      switch (expr.Op().type) {
        case lexer::OR: result = (lhs_value || rhs_value); break;
        case lexer::AND: result = (lhs_value && rhs_value); break;
        default: break;
      }

      auto new_bool_expr = make_shared<ast::BoolLitExpr>(
          lexer::Token(result ? lexer::K_TRUE : lexer::K_FALSE, ExtentOf(exprptr)), ast::TypeId::kBool);
      return make_shared<ConstExpr>(new_bool_expr, exprptr);
    }

    // Note: Includes chars.
    if (IsNumericOp(expr.Op().type)) {
      if (lhs_type == string_type_ || rhs_type == string_type_) {
        CHECK(expr.Op().type == lexer::ADD);

        jstring lhs_str = Stringify(lhs_const->ConstantPtr());
        jstring rhs_str = Stringify(rhs_const->ConstantPtr());
        jstring new_str = lhs_str + rhs_str;
        AddString(new_str);

        auto new_str_lit = make_shared<ast::StringLitExpr>(
            lexer::Token(lexer::STRING, ExtentOf(exprptr)),
            new_str, string_type_);
        return make_shared<ConstExpr>(new_str_lit, exprptr);
      }

      u32 lhs_value = (u32)GetIntValue(lhs_const->ConstantPtr());
      u32 rhs_value = (u32)GetIntValue(rhs_const->ConstantPtr());
      i32 result = 0;

      switch (expr.Op().type) {
        case lexer::ADD: result = lhs_value + rhs_value; break;
        case lexer::SUB: result = lhs_value - rhs_value; break;
        case lexer::MUL: result = lhs_value * rhs_value; break;
        case lexer::DIV:
          // If dividing by 0, don't fold constants.
          if (rhs_value == 0) {
            return exprptr;
          }
          result = (i32)lhs_value / (i32)rhs_value;
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
          lexer::Token(lexer::INTEGER, ExtentOf(exprptr)), (i64)result, ast::TypeId::kInt);
      return make_shared<ConstExpr>(new_int_expr, exprptr);

    }

    CHECK(IsRelationalOp(expr.Op().type) || IsEqualityOp(expr.Op().type));

    if (lhs_type == string_type_ && rhs_type == string_type_) {
      bool is_eq = expr.Op().type == lexer::EQ;
      CHECK(is_eq || expr.Op().type == lexer::NEQ);

      jstring lhs_str = Stringify(lhs_const->ConstantPtr());
      jstring rhs_str = Stringify(rhs_const->ConstantPtr());
      bool eq = (lhs_str == rhs_str);
      bool result = (eq && is_eq) || (!eq && !is_eq);
      auto new_bool_lit = make_shared<ast::BoolLitExpr>(
          lexer::Token(result ? lexer::K_TRUE : lexer::K_FALSE, ExtentOf(exprptr)),
          ast::TypeId::kBool);
      return make_shared<ConstExpr>(new_bool_lit, exprptr);
    }

    i64 lhs_value = GetIntValue(lhs_const->ConstantPtr());
    i64 rhs_value = GetIntValue(rhs_const->ConstantPtr());
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
    return make_shared<ConstExpr>(new_bool_expr, exprptr);
  }

  REWRITE_DECL(UnaryExpr, Expr, expr, exprptr) {
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

  REWRITE_DECL(CastExpr, Expr, expr, exprptr) {
    sptr<const Expr> new_inner = Rewrite(expr.GetExprPtr());

    ast::TypeId cast_type = expr.GetTypeId();
    ast::TypeId rhs_type = new_inner->GetTypeId();

    sptr<const Expr> new_cast_expr = make_shared<ast::CastExpr>(expr.Lparen(), expr.GetTypePtr(), expr.Rparen(), new_inner, cast_type);

    auto inner_const = dynamic_cast<const ConstExpr*>(new_inner.get());
    // Check that we are casting a constant.
    if (inner_const == nullptr) {
      return new_cast_expr;
    }

    // Propagate constant past identity cast.
    if (cast_type == rhs_type) {
      return make_shared<ConstExpr>(inner_const->ConstantPtr(), exprptr);
    }

    if (TypeChecker::IsPrimitive(cast_type)) {
      // Booleans can only be cast to strings or themselves, so these must be ints.
      CHECK(TypeChecker::IsNumeric(cast_type));

      auto inner_const_int = dynamic_cast<const ast::IntLitExpr*>(inner_const->ConstantPtr().get());
      CHECK(inner_const_int != nullptr);

      i64 value = inner_const_int->Value();
      u32 usigned = (u32)value;
      switch (cast_type.base) {
        case ast::TypeId::kIntBase:
          break;
        case ast::TypeId::kCharBase:
        case ast::TypeId::kShortBase:
          usigned = usigned & 0x0000FFFF;
          break;
        case ast::TypeId::kByteBase:
          usigned = usigned & 0x000000FF;
          break;
        default:
          UNREACHABLE();
      }
      i64 new_value = (i64)usigned;

      auto new_int_lit = make_shared<ast::IntLitExpr>(
          lexer::Token(lexer::INTEGER, ExtentOf(exprptr)),
          new_value, cast_type);
      return make_shared<ConstExpr>(new_int_lit, exprptr);
    }

    // Cast to String (for a constant type).
    // TODO: Is this even possible in the type system?
    CHECK(cast_type == string_type_);
    jstring str = Stringify(inner_const->ConstantPtr());
    AddString(str);

    auto new_str_lit = make_shared<ast::StringLitExpr>(
        lexer::Token(lexer::STRING, ExtentOf(exprptr)),
        str, string_type_);
    return make_shared<ConstExpr>(new_str_lit, exprptr);
  }

  ConstStringMap& strings_;
  ast::TypeId string_type_;
  StringId next_string_id_ = kFirstStringId;
};

sptr<const ast::Program> ConstantFold(sptr<const ast::Program> prog, ast::TypeId string_type, ConstStringMap* out_strings) {
  ConstantFoldingVisitor v(out_strings, string_type);
  return v.Rewrite(prog);
}


} // namespace types

