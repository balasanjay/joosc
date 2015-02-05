#ifndef WEEDER_STRUCTURE_VISITOR_H
#define WEEDER_STRUCTURE_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/recursive_visitor.h"

namespace weeder {

// StructureVisitor checks that the a compilation unit has at most 1 type
// declaration; it also verifies that a type declaration T is declared in a
// file T.java.
class StructureVisitor : public parser::RecursiveVisitor {
 public:
  StructureVisitor(const base::FileSet* fs, base::ErrorList* errors)
      : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(Program, prog);

 private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

}  // namespace weeder

#endif
