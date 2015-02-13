#include "weeder/weeder.h"

#include "weeder/assignment_visitor.h"
#include "weeder/call_visitor.h"
#include "weeder/int_range_visitor.h"
#include "weeder/modifier_visitor.h"
#include "weeder/structure_visitor.h"
#include "weeder/type_visitor.h"

using ast::Program;
using base::ErrorList;
using base::FileSet;

namespace weeder {

sptr<const Program> WeedProgram(const FileSet* fs, sptr<const Program> prog, ErrorList* out) {
  AssignmentVisitor assignmentChecker(fs, out);
  prog = Visit(&assignmentChecker, prog);

  CallVisitor callChecker(fs, out);
  prog->AcceptVisitor(&callChecker);

  TypeVisitor typeChecker(fs, out);
  prog = Visit(&typeChecker, prog);

  ModifierVisitor modifierChecker(fs, out);
  prog = Visit(&modifierChecker, prog);

  IntRangeVisitor intRangeChecker(fs, out);
  prog->AcceptVisitor(&intRangeChecker);

  StructureVisitor structureChecker(fs, out);
  prog->AcceptVisitor(&structureChecker);

  // More weeding required.

  return prog;
}

}  // namespace weeder
