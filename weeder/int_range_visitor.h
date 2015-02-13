#ifndef WEEDER_INT_RANGE_VISITOR_H
#define WEEDER_INT_RANGE_VISITOR_H

#include "ast/visitor2.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace weeder {

// IntRangeVisitor checks that int literals are in range.
// Joos ints are signed 32-bit integers.
class IntRangeVisitor : public ast::Visitor2 {
 public:
  IntRangeVisitor(const base::FileSet* fs, base::ErrorList* errors)
      : fs_(fs), errors_(errors) {}

  VISIT_DECL2(IntLitExpr, expr);
  VISIT_DECL2(UnaryExpr, expr);

 private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

}  // namespace weeder

#endif
