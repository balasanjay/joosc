#ifndef WEEDER_INT_RANGE_VISITOR_H
#define WEEDER_INT_RANGE_VISITOR_H

#include "ast/visitor.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace weeder {

// IntRangeVisitor checks that int literals are in range.
// Joos ints are signed 32-bit integers.
class IntRangeVisitor : public ast::Visitor {
 public:
  IntRangeVisitor(base::ErrorList* errors) : errors_(errors) {}

  VISIT_DECL(IntLitExpr, expr,);
  VISIT_DECL(UnaryExpr, expr,);

 private:
  base::ErrorList* errors_;
};

}  // namespace weeder

#endif
