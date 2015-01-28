#ifndef PARSER_ASSIGNMENT_VISITOR_H
#define PARSER_ASSIGNMENT_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/recursive_visitor.h"

// TODO: this needs to be moved to weeder.
namespace parser {

// AssignmentVisitor checks that the left-hand-side of an assignment is one of
// NameExpr, FieldDerefExpr, or ArrayIndexExpr.
class AssignmentVisitor : public RecursiveVisitor {
public:
  AssignmentVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(BinExpr, expr);

private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

} // namespace parser

#endif
