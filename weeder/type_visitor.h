#ifndef WEEDER_TYPE_VISITOR_H
#define WEEDER_TYPE_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/recursive_visitor.h"

namespace weeder {

// TypeVisitor checks two things.
//   1) "void" is only valid as the return type of a method.
//   2) NewClassExpr must have a non-primitive type. i.e. no "new int(1)".
class TypeVisitor : public parser::RecursiveVisitor {
public:
  TypeVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(CastExpr, expr);

  REC_VISIT_DECL(NewClassExpr, expr);
  REC_VISIT_DECL(NewArrayExpr, expr);

  REC_VISIT_DECL(LocalDeclStmt, stmt);

  REC_VISIT_DECL(FieldDecl, decl);
  REC_VISIT_DECL(Param, param);

private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

} // namespace weeder

#endif
