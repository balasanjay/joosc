#ifndef WEEDER_CALL_VISITOR_H
#define WEEDER_CALL_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/recursive_visitor.h"

namespace weeder {

// CallVisitor checks that the left-hand-side of a method call is one of
// NameExpr, or FieldDerefExpr.
class CallVisitor : public parser::RecursiveVisitor {
public:
  CallVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(CallExpr, expr);

private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

} // namespace weeder

#endif
