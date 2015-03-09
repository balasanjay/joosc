#ifndef WEEDER_CALL_VISITOR_H
#define WEEDER_CALL_VISITOR_H

#include "ast/visitor.h"
#include "base/errorlist.h"

namespace weeder {

// CallVisitor checks that the left-hand-side of a method call is one of
// NameExpr, or FieldDerefExpr.
class CallVisitor : public ast::Visitor {
 public:
  CallVisitor(base::ErrorList* errors) : errors_(errors) {}

  VISIT_DECL(CallExpr, expr,);

 private:
  base::ErrorList* errors_;
};

}  // namespace weeder

#endif
