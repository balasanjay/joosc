#include "ir/ir_generator.h"

#include <algorithm>

#include "ast/ast.h"
#include "ast/visitor.h"
#include "ir/stream_builder.h"
#include "lexer/lexer.h"

using ast::VisitResult;

namespace ir {

class MethodIRGenerator final : public ast::Visitor {
 public:
  MethodIRGenerator(Mem res, bool lvalue, StreamBuilder& builder, vector<ast::LocalVarId>& locals, map<ast::LocalVarId, Mem>& locals_map): res_(res), lvalue_(lvalue), builder_(builder), locals_(locals), locals_map_(locals_map)  {}

  VISIT_DECL(MethodDecl, decl,) {
    // TODO: Calling semantics.
    Visit(decl.BodyPtr());
    return VisitResult::SKIP;
  }

  VISIT_DECL(BlockStmt, stmt,) {
    vector<ast::LocalVarId> block_locals;
    MethodIRGenerator gen(res_, false, builder_, block_locals, locals_map_);
    for (int i = 0; i < stmt.Stmts().Size(); ++i) {
      auto st = stmt.Stmts().At(i);
      gen.Visit(st);
    }

    // Have the Mems deallocated in order of allocation, so we maintain stack
    // invariant.
    std::reverse(block_locals.begin(), block_locals.end());
    for (auto vid : block_locals) {
      locals_map_.erase(vid);
    }

    return VisitResult::SKIP;
  }

  VISIT_DECL(BinExpr, expr,) {
    SizeClass size = SizeClass::INT;
    bool is_assg = false;
    if (lexer::IsNumericOp(expr.Op().type)) {
      size = SizeClass::INT;
    } else if (lexer::IsBoolOp(expr.Op().type)) {
      size = SizeClass::BOOL;
    } else if (expr.Op().type == lexer::ASSG) {
      is_assg = true;
      ast::TypeId tid = expr.Lhs().GetTypeId();
      if (tid == ast::TypeId::kInt) {
        size = SizeClass::INT;
      } else if (tid == ast::TypeId::kBool) {
        size = SizeClass::BOOL;
      }
    }

    Mem lhs = builder_.AllocTemp(size);
    Mem rhs = builder_.AllocTemp(size);
    {
      MethodIRGenerator gen(lhs, is_assg, builder_, locals_, locals_map_);
      gen.Visit(expr.LhsPtr());
    }
    {
      MethodIRGenerator gen(rhs, false, builder_, locals_, locals_map_);
      gen.Visit(expr.RhsPtr());
    }

    if (is_assg) {
      builder_.Mov(lhs, rhs);
      return VisitResult::SKIP;
    }

#define C(fn) builder_.fn(res_, lhs, rhs); break;
    switch (expr.Op().type) {
      case lexer::ADD: C(Add);
      case lexer::EQ:  C(Eq);
      case lexer::NEQ: C(Neq);
      case lexer::LT:  C(Lt);
      case lexer::LE:  C(Leq);
      case lexer::GT:  C(Gt);
      case lexer::GE:  C(Geq);
      default: break;
    }
#undef C

    return VisitResult::SKIP;
  }

  VISIT_DECL(IntLitExpr, expr,) {
    // TODO: Ensure no overflow.
    Mem m = builder_.ConstInt32((i32)expr.Value());
    builder_.Mov(res_, m);
    return VisitResult::SKIP;
  }

  VISIT_DECL(BoolLitExpr, expr,) {
    Mem m = builder_.ConstBool(expr.GetToken().type == lexer::K_TRUE);
    builder_.Mov(res_, m);
    return VisitResult::SKIP;
  }

  VISIT_DECL(NameExpr, expr,) {
    auto i = locals_map_.find(expr.GetVarId());
    CHECK(i != locals_map_.end());
    Mem local = i->second;
    if (lvalue_) {
      builder_.MovAddr(res_, local);
    } else {
      builder_.Mov(res_, local);
    }
    return VisitResult::SKIP;
  }

  VISIT_DECL(LocalDeclStmt, stmt,) {
    Mem local = builder_.AllocLocal(SizeClass::INT);
    locals_.push_back(stmt.GetVarId());
    locals_map_.insert({stmt.GetVarId(), local});

    MethodIRGenerator gen(local, false, builder_, locals_, locals_map_);
    gen.Visit(stmt.GetExprPtr());

    return VisitResult::SKIP;
  }

  // Location result of the computation should be stored.
  Mem res_;
  bool lvalue_ = false;
  StreamBuilder& builder_;
  vector<ast::LocalVarId>& locals_;
  map<ast::LocalVarId, Mem>& locals_map_;
};

class ProgramIRGenerator final : public ast::Visitor {
 public:
  ProgramIRGenerator() {}
  VISIT_DECL(CompUnit, unit, ) {
    stringstream ss;
    ss << 'f' << unit.FileId() << ".s";
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
    StreamBuilder builder;
    vector<ast::LocalVarId> empty_locals;
    map<ast::LocalVarId, Mem> locals_map;
    {
      Mem ret = builder.AllocTemp(SizeClass::INT);

      MethodIRGenerator gen(ret, false, builder, empty_locals, locals_map);
      gen.Visit(declptr);
    }
    // Return mem must be deallocated before Build is called.

    current_unit_.streams.push_back(builder.Build(true, 2, 2));
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


