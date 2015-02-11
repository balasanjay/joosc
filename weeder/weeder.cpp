#include "weeder/assignment_visitor.h"
#include "weeder/call_visitor.h"
#include "weeder/int_range_visitor.h"
#include "weeder/modifier_visitor.h"
#include "weeder/structure_visitor.h"
#include "weeder/type_visitor.h"
#include "weeder/weeder.h"

using ast::Program;
using base::ErrorList;
using base::FileSet;

namespace weeder {

void WeedProgram(const FileSet* fs, const Program* prog, ErrorList* out) {
  AssignmentVisitor assignmentChecker(fs, out);
  prog->AcceptVisitor(&assignmentChecker);

  CallVisitor callChecker(fs, out);
  prog->AcceptVisitor(&callChecker);

  TypeVisitor typeChecker(fs, out);
  prog->AcceptVisitor(&typeChecker);

  ModifierVisitor modifierChecker(fs, out);
  prog->AcceptVisitor(&modifierChecker);

  IntRangeVisitor intRangeChecker(fs, out);
  prog->AcceptVisitor(&intRangeChecker);

  StructureVisitor structureChecker(fs, out);
  prog->AcceptVisitor(&structureChecker);

  // More weeding required.
}

}  // namespace weeder
