#include "weeder/assignment_visitor.h"
#include "weeder/call_visitor.h"
#include "weeder/int_range_visitor.h"
#include "weeder/modifier_visitor.h"
#include "weeder/structure_visitor.h"
#include "weeder/type_visitor.h"
#include "weeder/weeder.h"

using base::ErrorList;
using base::FileSet;
using parser::Program;

namespace weeder {

void WeedProgram(const FileSet* fs, const Program* prog, ErrorList* out) {
  AssignmentVisitor assignmentChecker(fs, out);
  prog->Accept(&assignmentChecker);

  CallVisitor callChecker(fs, out);
  prog->Accept(&callChecker);

  TypeVisitor typeChecker(fs, out);
  prog->Accept(&typeChecker);

  ModifierVisitor modifierChecker(fs, out);
  prog->Accept(&modifierChecker);

  IntRangeVisitor intRangeChecker(fs, out);
  prog->Accept(&intRangeChecker);

  StructureVisitor structureChecker(fs, out);
  prog->Accept(&structureChecker);

  // More weeding required.
}

} // namespace weeder
