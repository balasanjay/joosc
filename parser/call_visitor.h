#ifndef PARSER_CALL_VISITOR_H
#define PARSER_CALL_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/recursive_visitor.h"

// TODO: this needs to be moved to weeder.
namespace parser {

// CallVisitor checks that the left-hand-side of a method call is one of
// NameExpr, or FieldDerefExpr.
class CallVisitor : public RecursiveVisitor {
public:
  CallVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(CallExpr, expr);

private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

} // namespace parser

#endif
