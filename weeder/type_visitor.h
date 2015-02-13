#ifndef WEEDER_TYPE_VISITOR_H
#define WEEDER_TYPE_VISITOR_H

#include "ast/visitor2.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace weeder {

// TypeVisitor checks the following things.
//   1) "void" is only valid as the return type of a method.
//   2) NewClassExpr must have a non-primitive type. i.e. no "new int(1)".
//   3) RHS of an instanceof must be a NameExpr.
class TypeVisitor : public ast::Visitor {
 public:
  TypeVisitor(const base::FileSet* fs, base::ErrorList* errors)
      : fs_(fs), errors_(errors) {}

  VISIT_DECL(CastExpr, expr);
  VISIT_DECL(InstanceOfExpr, expr);

  VISIT_DECL(NewClassExpr, expr);
  VISIT_DECL(NewArrayExpr, expr);

  VISIT_DECL(LocalDeclStmt, stmt);

  VISIT_DECL(FieldDecl, decl);
  VISIT_DECL(Param, param);

  VISIT_DECL(ForStmt, stmt);
  VISIT_DECL(BlockStmt, stmt);

 private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

}  // namespace weeder

#endif
