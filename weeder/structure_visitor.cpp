#include "weeder/structure_visitor.h"

#include "ast/ast.h"
#include "base/macros.h"
#include "lexer/lexer.h"

using ast::CompUnit;
using ast::Expr;
using ast::FieldDerefExpr;
using ast::NameExpr;
using ast::Program;
using ast::VisitResult;
using base::Error;
using base::File;
using lexer::ASSG;
using lexer::Token;

namespace weeder {
namespace {

Error* MakeMultipleTypesPerCompUnitError(Token token) {
  return MakeSimplePosRangeError(
      token.pos, "MultipleTypesPerCompUnitError",
      "Joos does not support multiple types per file.");
}

}  // namespace

VISIT_DEFN(StructureVisitor, Program, prog,) {
  CHECK(prog.CompUnits().Size() == fs_->Size());

  for (int i = 0; i < prog.CompUnits().Size(); ++i) {
    const File* file = fs_->Get(i);
    const CompUnit* unit = prog.CompUnits().At(i).get();
    CHECK(unit->FileId() == i);

    if (unit->Types().Size() > 1) {
      for (int j = 0; j < unit->Types().Size(); ++j) {
        errors_->Append(MakeMultipleTypesPerCompUnitError(
            unit->Types().At(j)->NameToken()));
      }
      continue;
    } else if (unit->Types().Size() == 0) {
      continue;
    }

    CHECK(unit->Types().Size() == 1);
    string typeName = unit->Types().At(0)->Name();
    string expectedFilename = typeName + ".java";

    if (file->Basename() == expectedFilename) {
      continue;
    }

    errors_->Append(MakeSimplePosRangeError(
        unit->Types().At(0)->NameToken().pos, "IncorrectFileNameError",
        "Must be in file named " + expectedFilename + "."));
  }

  return VisitResult::SKIP;
}

}  // namespace weeder
