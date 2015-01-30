#include "base/macros.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "weeder/structure_visitor.h"

using base::Error;
using base::File;
using base::FileSet;
using lexer::ASSG;
using lexer::Token;
using parser::CompUnit;
using parser::Expr;
using parser::FieldDerefExpr;
using parser::NameExpr;
using parser::Program;

namespace weeder {
namespace {

Error* MakeMultipleTypesPerCompUnitError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "MultipleTypesPerCompUnitError", "Joos does not support multiple types per file.");
}

} // namespace

REC_VISIT_DEFN(StructureVisitor, Program, prog){
  // TODO: store fileid in CompUnit, and use that instead of this assertion.
  assert(prog->CompUnits().Size() == fs_->Size());

  for (int i = 0; i < prog->CompUnits().Size(); ++i) {
    const File* file = fs_->Get(i);
    const CompUnit* unit = prog->CompUnits().At(i);

    if (unit->Types().Size() > 1) {
      for (int j = 0; j < unit->Types().Size(); ++j) {
        errors_->Append(MakeMultipleTypesPerCompUnitError(fs_, unit->Types().At(j)->NameToken()));
      }
      continue;
    } else if (unit->Types().Size() == 0) {
      continue;
    }

    assert(unit->Types().Size() == 1);
    string typeName = unit->Types().At(0)->Name();
    string expectedFilename = typeName + ".java";

    if (file->Basename() == expectedFilename) {
      continue;
    }

    errors_->Append(MakeSimplePosRangeError(
          fs_, unit->Types().At(0)->NameToken().pos,
          "IncorrectFileNameError", "Must be in file named " + expectedFilename + "."));
  }

  return false;
}

} // namespace weeder
