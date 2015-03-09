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
  AssignmentVisitor assignmentChecker(out);
  prog = assignmentChecker.Rewrite(prog);

  CallVisitor callChecker(out);
  prog = callChecker.Rewrite(prog);

  TypeVisitor typeChecker(out);
  prog = typeChecker.Rewrite(prog);

  ModifierVisitor modifierChecker(out);
  prog = modifierChecker.Rewrite(prog);

  IntRangeVisitor intRangeChecker(out);
  prog = intRangeChecker.Rewrite(prog);

  StructureVisitor structureChecker(fs, out);
  prog = structureChecker.Rewrite(prog);

  return prog;
}

}  // namespace weeder
