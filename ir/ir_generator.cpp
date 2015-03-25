#include "ir/ir_generator.h"

#include "ast/ast.h"
#include "ast/visitor.h"
#include "ir/stream_builder.h"

using ast::VisitResult;

namespace ir {

class MethodIRGenerator final : public ast::Visitor {
 public:
  MethodIRGenerator() {}

  VISIT_DECL(MethodDecl, decl, declptr) {
    Mem m = builder_.ConstInt32(1);
    Mem m2 = builder_.ConstInt32(2);
    Mem tmp = builder_.AllocTemp(SizeClass::INT);
    builder_.Add(tmp, m, m2);
    return VisitResult::SKIP;
  }

  StreamBuilder builder_;
};

class ProgramIRGenerator final : public ast::Visitor {
 public:
  ProgramIRGenerator() {}
  VISIT_DECL(CompUnit, unit, ) {
    stringstream ss;
    ss << unit.FileId();
    current_unit_.filename = ss.str();

    for (int i = 0; i < unit.Types().Size(); ++i) {
      Visit(unit.Types().At(i));
    }
    prog.units.push_back(current_unit_);
    current_unit_ = ir::CompUnit();
    return VisitResult::SKIP;
  }

  VISIT_DECL(MethodDecl, decl, declptr) {
    if (decl.Name() != "test") {
      // TODO.
      return VisitResult::SKIP;
    }
    MethodIRGenerator gen;
    gen.Visit(declptr);
    current_unit_.streams.push_back(gen.builder_.Build(true, 2, 2));
    return VisitResult::SKIP;
  }

  ir::Program prog;

 private:
  ir::CompUnit current_unit_;
};

Program GenerateIR(sptr<const ast::Program> program) {
  ProgramIRGenerator gen;
  gen.Visit(program);
  return gen.prog;
}


} // namespace ir


