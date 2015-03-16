#ifndef WEEDER_INT_RANGE_VISITOR_H
#define WEEDER_INT_RANGE_VISITOR_H

#include "ast/visitor.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "lexer/lexer.h"

namespace weeder {

// IntRangeVisitor checks that int literals are in range.
// Joos ints are signed 32-bit integers.
class IntRangeVisitor : public ast::Visitor {
 public:
  IntRangeVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REWRITE_DECL(IntLitExpr, Expr, expr,);
  REWRITE_DECL(UnaryExpr, Expr, expr,);

 private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

}  // namespace weeder

#endif
