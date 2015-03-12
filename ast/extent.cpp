#include "ast/extent.h"

#include "ast/ast.h"
#include "ast/visitor.h"
#include "base/macros.h"

using base::PosRange;

namespace ast {

class ExtentVisitor final : public Visitor {
 public:
  ExtentVisitor() = default;

  PosRange Extent() const { return extent_; }

  VISIT_DECL(ArrayIndexExpr, expr,) {
    Visit(expr.BasePtr());
    UpdatePos(expr.Rbrack().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(CallExpr, expr,) {
    Visit(expr.BasePtr());
    UpdatePos(expr.Rparen().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(CastExpr, expr,) {
    UpdatePos(expr.Lparen().pos);
    Visit(expr.GetExprPtr());
    return VisitResult::SKIP;
  }

  VISIT_DECL(InstanceOfExpr, expr,) {
    Visit(expr.LhsPtr());
    UpdatePosFromType(expr.GetType());
    return VisitResult::SKIP;
  }

  VISIT_DECL(FieldDerefExpr, expr,) {
    Visit(expr.BasePtr());
    UpdatePos(expr.GetToken().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(BoolLitExpr, expr,) {
    UpdatePos(expr.GetToken().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(StringLitExpr, expr,) {
    UpdatePos(expr.GetToken().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(CharLitExpr, expr,) {
    UpdatePos(expr.GetToken().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(NullLitExpr, expr,) {
    UpdatePos(expr.GetToken().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(IntLitExpr, expr,) {
    UpdatePos(expr.GetToken().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(NameExpr, expr,) {
    const vector<lexer::Token> toks = expr.Name().Tokens();
    UpdatePos(toks.begin()->pos);
    UpdatePos(toks.rbegin()->pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(StaticRefExpr, expr,) {
    UpdatePosFromType(expr.GetRefType());
    return VisitResult::SKIP;
  }

  VISIT_DECL(NewArrayExpr, expr,) {
    UpdatePos(expr.NewToken().pos);
    UpdatePos(expr.Rbrack().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(NewClassExpr, expr,) {
    UpdatePos(expr.NewToken().pos);
    UpdatePos(expr.Rparen().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(ParenExpr, expr,) {
    UpdatePos(expr.Lparen().pos);
    UpdatePos(expr.Rparen().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(ThisExpr, expr,) {
    UpdatePos(expr.ThisToken().pos);
    return VisitResult::SKIP;
  }

  VISIT_DECL(UnaryExpr, expr,) {
    UpdatePos(expr.Op().pos);
    Visit(expr.RhsPtr());
    return VisitResult::SKIP;
  }

 private:
  void UpdatePosFromType(const Type& type) {
    if (IS_CONST_REF(ArrayType, type)) {
      const ArrayType& arr = dynamic_cast<const ArrayType&>(type);
      UpdatePosFromType(arr.ElemType());
      UpdatePos(arr.Rbrack().pos);
      return;
    }
    if (IS_CONST_REF(PrimitiveType, type)) {
      const PrimitiveType& prim = dynamic_cast<const PrimitiveType&>(type);
      UpdatePos(prim.GetToken().pos);
      return;
    }

    CHECK(IS_CONST_REF(ReferenceType, type));
    const ReferenceType& ref = dynamic_cast<const ReferenceType&>(type);
    const vector<lexer::Token> toks = ref.Name().Tokens();
    UpdatePos(toks.begin()->pos);
    UpdatePos(toks.rbegin()->pos);
  }

  void UpdatePositions(std::initializer_list<PosRange> positions) {
    for (const auto& pos : positions) {
      UpdatePos(pos);
    }
  }

  void UpdatePos(PosRange pos) {
    if (extent_.fileid == -1) {
      extent_ = pos;
      return;
    }
    CHECK(extent_.fileid == pos.fileid);
    extent_.begin = std::min(extent_.begin, pos.begin);
    extent_.end = std::max(extent_.end, pos.end);
  }

  PosRange extent_ = {-1, -1, -1};
};

PosRange ExtentOf(sptr<const Expr> expr) {
  ExtentVisitor visitor;
  visitor.Visit(expr);
  CHECK(visitor.Extent().fileid != -1);
  return visitor.Extent();
}

PosRange ExtentOf(sptr<const Stmt> stmt) {
  ExtentVisitor visitor;
  visitor.Visit(stmt);
  CHECK(visitor.Extent().fileid != -1);
  return visitor.Extent();
}


} // namespace ast

