#include "weeder/assignment_visitor.h"
#include "weeder/call_visitor.h"
#include "weeder/modifier_visitor.h"
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

  // More weeding required.
}

} // namespace weeder
