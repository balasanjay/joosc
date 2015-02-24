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
  prog = assignmentChecker.Rewrite(prog);

  CallVisitor callChecker(fs, out);
  prog = callChecker.Rewrite(prog);

  TypeVisitor typeChecker(fs, out);
  prog = typeChecker.Rewrite(prog);

  ModifierVisitor modifierChecker(fs, out);
  prog = modifierChecker.Rewrite(prog);

  IntRangeVisitor intRangeChecker(fs, out);
  prog = intRangeChecker.Rewrite(prog);

  StructureVisitor structureChecker(fs, out);
  prog = structureChecker.Rewrite(prog);

  // More weeding required.

  return prog;
}

}  // namespace weeder
