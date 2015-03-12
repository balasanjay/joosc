#ifndef CONSTANT_FOLDING_VISITOR_H
#define CONSTANT_FOLDING_VISITOR_H

#include "ast/visitor.h"
#include "types/type_info_map.h"

namespace types {

class ConstantFoldingVisitor final : public ast::Visitor {
 public:
  ConstantFoldingVisitor() {}

  //REWRITE_DECL(StringLitExpr, Expr,,);
  //REWRITE_DECL(CharLitExpr, Expr,,);
  REWRITE_DECL(IntLitExpr, Expr,,);
  REWRITE_DECL(BoolLitExpr, Expr,,);
  REWRITE_DECL(BinExpr, Expr,,);
  REWRITE_DECL(UnaryExpr, Expr,,);
};

} // namespace types

#endif
