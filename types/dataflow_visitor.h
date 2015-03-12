#ifndef DATAFLOW_VISITOR_H
#define DATAFLOW_VISITOR_H

#include "ast/visitor.h"
#include "base/errorlist.h"
#include "types/type_info_map.h"

namespace types {

class DataflowVisitor final : public ast::Visitor {
 public:
  DataflowVisitor(const TypeInfoMap& typeinfo, base::ErrorList* errors, ast::TypeId curtype = ast::TypeId::kUnassigned) : typeinfo_(typeinfo), errors_(errors), curtype_(curtype) {}

  VISIT_DECL(TypeDecl, MemberDecl, decl);
  VISIT_DECL(FieldDecl, MemberDecl, decl);
  VISIT_DECL(MethodDecl, MemberDecl, decl);

 private:
  const TypeInfoMap& typeinfo_;
  base::ErrorList* errors_;
  ast::TypeId curtype_ = ast::TypeId::kUnassigned;
};

} // namespace types

#endif
