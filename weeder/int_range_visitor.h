#ifndef WEEDER_INT_RANGE_VISITOR_H
#define WEEDER_INT_RANGE_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/recursive_visitor.h"

namespace weeder {

// IntRangeVisitor checks that int literals are in range.
// Ints must be within 32-bit signed two's complement.
class IntRangeVisitor: public parser::RecursiveVisitor {
public:
  IntRangeVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(IntLitExpr, expr);
  REC_VISIT_DECL(UnaryExpr, expr);

private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

} // namespace weeder

#endif
