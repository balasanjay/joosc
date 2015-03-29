#include "ir/ir_generator.h"

#include <algorithm>
#include <tuple>

#include "ast/ast.h"
#include "ast/extent.h"
#include "ast/visitor.h"
#include "ir/size.h"
#include "ir/stream_builder.h"
#include "lexer/lexer.h"
#include "types/type_info_map.h"

using std::dynamic_pointer_cast;
using std::get;
using std::tuple;

using ast::Expr;
using ast::ExtentOf;
using ast::FieldDecl;
using ast::FieldId;
using ast::MemberDecl;
using ast::MethodDecl;
using ast::MethodId;
using ast::StaticRefExpr;
using ast::TypeId;
using ast::TypeKind;
using ast::VisitResult;
using ast::kInitMethodId;
using ast::kVarImplicitThis;
using base::PosRange;
using types::TypeIdList;
using types::TypeInfo;
using types::TypeInfoMap;

namespace ir {

namespace {

class MethodIRGenerator final : public ast::Visitor {
 public:
  MethodIRGenerator(Mem res, bool lvalue, StreamBuilder* builder, vector<ast::LocalVarId>* locals, map<ast::LocalVarId, Mem>* locals_map, TypeId tid): res_(res), lvalue_(lvalue), builder_(*builder), locals_(*locals), locals_map_(*locals_map), tid_(tid) {}

  MethodIRGenerator WithResultIn(Mem res, bool lvalue=false) {
    return MethodIRGenerator(res, lvalue, &builder_, &locals_, &locals_map_, tid_);
  }

  MethodIRGenerator WithLocals(vector<ast::LocalVarId>& locals) {
    return MethodIRGenerator(res_, lvalue_, &builder_, &locals, &locals_map_, tid_);
  }

  VISIT_DECL(MethodDecl, decl,) {
    // Get param sizes.
    auto params = decl.Params().Params();
    vector<SizeClass> param_sizes;
    bool is_static = decl.Mods().HasModifier(lexer::STATIC);
    if (!is_static) {
      param_sizes.push_back(SizeClass::PTR);
    }

    for (int i = 0; i < params.Size(); ++i) {
      param_sizes.push_back(SizeClassFrom(params.At(i)->GetType().GetTypeId()));
    }

    // Allocate params.
    vector<Mem> param_mems;
    builder_.AllocParams(param_sizes, &param_mems);

    // Constructors call the init method, passing ``this'' as the only
    // argument.
    if (decl.TypePtr() == nullptr) {
      builder_.StaticCall(res_, tid_.base, kInitMethodId, {param_mems[0]});
    }

    // Add params to local map.
    for (int i = 0; i < params.Size(); ++i) {
      int idx = is_static ? i : i + 1;
      locals_map_.insert({params.At(i)->GetVarId(), param_mems.at(idx)});
    }

    if (!is_static) {
      locals_map_.insert({kVarImplicitThis, param_mems[0]});
    }

    Visit(decl.BodyPtr());
    // Param Mems will be deallocated when map is deallocated.

    return VisitResult::SKIP;
  }

  VISIT_DECL(BlockStmt, stmt,) {
    vector<ast::LocalVarId> block_locals;
    MethodIRGenerator gen = WithLocals(block_locals);
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

  VISIT_DECL(UnaryExpr, expr,) {
    if (expr.Op().type == lexer::SUB) {
      Mem rhs = builder_.AllocTemp(SizeClass::INT);
      WithResultIn(rhs).Visit(expr.RhsPtr());
      builder_.Neg(res_, rhs);
      return VisitResult::SKIP;
    }

    CHECK(expr.Op().type == lexer::NOT);
    Mem rhs = builder_.AllocTemp(SizeClass::BOOL);
    WithResultIn(rhs).Visit(expr.RhsPtr());
    builder_.Not(res_, rhs);

    return VisitResult::SKIP;
  }

  VISIT_DECL(BinExpr, expr,) {
    SizeClass size = SizeClassFrom(expr.Lhs().GetTypeId());

    // Special code for short-circuiting boolean and or.
    if (expr.Op().type == lexer::AND) {
      Mem lhs = builder_.AllocLocal(size);
      Mem rhs = builder_.AllocTemp(size);
      WithResultIn(lhs).Visit(expr.LhsPtr());

      LabelId short_circuit = builder_.AllocLabel();
      {
        // Short circuit 'and' with a false result if lhs is false.
        Mem not_lhs = builder_.AllocLocal(SizeClass::BOOL);
        builder_.Not(not_lhs, lhs);
        builder_.JmpIf(short_circuit, not_lhs);
      }

      // Rhs code.
      WithResultIn(rhs).Visit(expr.RhsPtr());
      // Using lhs as answer.
      builder_.Mov(lhs, rhs);

      builder_.EmitLabel(short_circuit);
      builder_.Mov(res_, lhs);

      return VisitResult::SKIP;

    } else if (expr.Op().type == lexer::OR) {
      Mem lhs = builder_.AllocLocal(size);
      Mem rhs = builder_.AllocTemp(size);
      WithResultIn(lhs).Visit(expr.LhsPtr());

      // Short circuit 'or' with a true result if lhs is true.
      LabelId short_circuit = builder_.AllocLabel();
      builder_.JmpIf(short_circuit, lhs);

      // Rhs code.
      WithResultIn(rhs).Visit(expr.RhsPtr());
      builder_.Mov(lhs, rhs);

      // Short circuit code.
      builder_.EmitLabel(short_circuit);
      builder_.Mov(res_, lhs);

      return VisitResult::SKIP;
    }

    bool is_assg = false;
    if (expr.Op().type == lexer::ASSG) {
      is_assg = true;
    }

    Mem lhs = builder_.AllocTemp(is_assg ? SizeClass::PTR : size);
    WithResultIn(lhs, is_assg).Visit(expr.LhsPtr());
    Mem rhs = builder_.AllocTemp(size);
    WithResultIn(rhs).Visit(expr.RhsPtr());

    if (is_assg) {
      builder_.MovToAddr(lhs, rhs);

      // The result of an assignment expression is the rhs. We don't bother
      // with this if it's in a top-level context.
      if (res_.IsValid()) {
        builder_.Mov(res_, rhs);
      }
      return VisitResult::SKIP;
    }

#define C(fn) builder_.fn(res_, lhs, rhs); break;
    switch (expr.Op().type) {
      case lexer::ADD:  C(Add);
      case lexer::SUB:  C(Sub);
      case lexer::MUL:  C(Mul);
      case lexer::DIV:  C(Div);
      case lexer::MOD:  C(Mod);
      case lexer::EQ:   C(Eq);
      case lexer::NEQ:  C(Neq);
      case lexer::LT:   C(Lt);
      case lexer::LE:   C(Leq);
      case lexer::GT:   C(Gt);
      case lexer::GE:   C(Geq);
      case lexer::BAND: C(And);
      case lexer::BOR:  C(Or);
      case lexer::XOR:  C(Xor);
      default: break;
    }
#undef C

    return VisitResult::SKIP;
  }

  VISIT_DECL(IntLitExpr, expr,) {
    // TODO: Ensure no overflow.
    builder_.ConstInt32(res_, (i32)expr.Value());
    return VisitResult::SKIP;
  }

  VISIT_DECL(BoolLitExpr, expr,) {
    builder_.ConstBool(res_, expr.GetToken().type == lexer::K_TRUE);
    return VisitResult::SKIP;
  }

  VISIT_DECL(NullLitExpr,,) {
    builder_.ConstNull(res_);
    return VisitResult::SKIP;
  }

  VISIT_DECL(ThisExpr,,) {
    auto i = locals_map_.find(kVarImplicitThis);
    CHECK(i != locals_map_.end());
    builder_.Mov(res_, i->second);
    return VisitResult::SKIP;
  }

  VISIT_DECL(FieldDerefExpr, expr,) {
    // TODO: handle static fields.
    {
      auto static_base = dynamic_cast<const StaticRefExpr*>(expr.BasePtr().get());
      if (static_base != nullptr) {
        return VisitResult::SKIP;
      }
    }

    Mem tmp = builder_.AllocTemp(SizeClass::PTR);
    // We want an rvalue of the pointer, so set lvalue to false.
    WithResultIn(tmp, false).Visit(expr.BasePtr());

    if (lvalue_) {
      builder_.FieldAddr(res_, tmp, expr.GetFieldId(), expr.GetToken().pos);
    } else {
      builder_.Field(res_, tmp, expr.GetFieldId(), expr.GetToken().pos);
    }

    return VisitResult::SKIP;
  }

  VISIT_DECL(ArrayIndexExpr, expr, exprptr) {
    Mem array = builder_.AllocTemp(SizeClass::PTR);

    // We want an rvalue of the pointer, so set lvalue to false.
    WithResultIn(array, false).Visit(expr.BasePtr());

    Mem index = builder_.AllocTemp(SizeClass::INT);
    WithResultIn(index).Visit(expr.IndexPtr());

    PosRange pos = ExtentOf(exprptr);
    SizeClass elemsize = SizeClassFrom(expr.GetTypeId());
    if (lvalue_) {
      builder_.ArrayAddr(res_, array, index, elemsize, pos);
    } else {
      builder_.ArrayDeref(res_, array, index, elemsize, pos);
    }

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

  VISIT_DECL(ReturnStmt, stmt,) {
    if (stmt.GetExprPtr() == nullptr) {
      builder_.Ret();
      return VisitResult::SKIP;
    }

    Mem ret = builder_.AllocTemp(SizeClassFrom(stmt.GetExprPtr()->GetTypeId()));
    WithResultIn(ret).Visit(stmt.GetExprPtr());
    builder_.Ret(ret);

    return VisitResult::SKIP;
  }

  VISIT_DECL(LocalDeclStmt, stmt,) {
    ast::TypeId tid = stmt.GetType().GetTypeId();
    Mem local = builder_.AllocLocal(SizeClassFrom(tid));
    locals_.push_back(stmt.GetVarId());
    locals_map_.insert({stmt.GetVarId(), local});

    WithResultIn(local).Visit(stmt.GetExprPtr());

    return VisitResult::SKIP;
  }

  VISIT_DECL(IfStmt, stmt,) {
    Mem cond = builder_.AllocTemp(SizeClass::BOOL);
    WithResultIn(cond).Visit(stmt.CondPtr());

    LabelId begin_false = builder_.AllocLabel();
    LabelId after_if = builder_.AllocLabel();

    Mem not_cond = builder_.AllocTemp(SizeClass::BOOL);
    builder_.Not(not_cond, cond);
    builder_.JmpIf(begin_false, not_cond);

    // Emit true body code.
    Visit(stmt.TrueBodyPtr());
    builder_.Jmp(after_if);

    // Emit false body code.
    builder_.EmitLabel(begin_false);
    Visit(stmt.FalseBodyPtr());

    builder_.EmitLabel(after_if);

    return VisitResult::SKIP;
  }

  VISIT_DECL(WhileStmt, stmt,) {
    // Top of loop label.
    LabelId loop_begin = builder_.AllocLabel();
    LabelId loop_end = builder_.AllocLabel();
    builder_.EmitLabel(loop_begin);

    // Condition code.
    Mem cond = builder_.AllocTemp(SizeClass::BOOL);
    WithResultIn(cond).Visit(stmt.CondPtr());

    // Leave loop if condition is false.
    Mem not_cond = builder_.AllocTemp(SizeClass::BOOL);
    builder_.Not(not_cond, cond);
    builder_.JmpIf(loop_end, not_cond);

    // Loop body.
    Visit(stmt.BodyPtr());

    // Loop back to first label.
    builder_.Jmp(loop_begin);

    builder_.EmitLabel(loop_end);

    return VisitResult::SKIP;
  }

  VISIT_DECL(ForStmt, stmt,) {
    // Scope initializer variable.
    vector<ast::LocalVarId> loop_locals;
    {
      // Do initialization.
      MethodIRGenerator gen = WithLocals(loop_locals);
      gen.Visit(stmt.InitPtr());

      LabelId loop_begin = builder_.AllocLabel();
      LabelId loop_end = builder_.AllocLabel();

      builder_.EmitLabel(loop_begin);

      // Condition code.
      if (stmt.CondPtr() != nullptr) {
        Mem cond = builder_.AllocTemp(SizeClass::BOOL);
        gen.WithResultIn(cond).Visit(stmt.CondPtr());

        // Leave loop if condition is false.
        Mem not_cond = builder_.AllocTemp(SizeClass::BOOL);
        builder_.Not(not_cond, cond);
        builder_.JmpIf(loop_end, not_cond);
      }

      // Loop body.
      Visit(stmt.BodyPtr());

      // Loop update.
      if (stmt.UpdatePtr() != nullptr) {
        Visit(stmt.UpdatePtr());
      }

      // Loop back to first label.
      builder_.Jmp(loop_begin);

      builder_.EmitLabel(loop_end);
    }

    // Have the Mems deallocated in order of allocation, so we maintain stack
    // invariant.
    std::reverse(loop_locals.begin(), loop_locals.end());
    for (auto vid : loop_locals) {
      locals_map_.erase(vid);
    }
    return VisitResult::SKIP;
  }

  VISIT_DECL(CallExpr, expr,) {
    // Allocate argument temps and generate their code.
    vector<Mem> arg_mems;
    for (int i = 0; i < expr.Args().Size(); ++i) {
      auto arg = expr.Args().At(i);
      Mem arg_mem = builder_.AllocTemp(SizeClassFrom(arg->GetTypeId()));
      WithResultIn(arg_mem).Visit(arg);
      arg_mems.push_back(arg_mem);
    }

    auto static_base = dynamic_cast<const StaticRefExpr*>(expr.BasePtr().get());
    if (static_base == nullptr) {
      Mem this_ptr = builder_.AllocTemp(SizeClass::PTR);
      WithResultIn(this_ptr).Visit(expr.BasePtr());

      builder_.DynamicCall(res_, this_ptr, expr.GetMethodId(), arg_mems);
    } else {
      TypeId tid = static_base->GetRefType().GetTypeId();
      builder_.StaticCall(res_, tid.base, expr.GetMethodId(), arg_mems);
    }

    // Deallocate arg mems.
    while (!arg_mems.empty()) {
      arg_mems.pop_back();
    }

    return VisitResult::SKIP;
  }

  VISIT_DECL(NewArrayExpr, expr,) {
    // TODO: handle optional expr.
    if (expr.GetExprPtr() == nullptr) {
      return VisitResult::SKIP;
    }

    Mem size = builder_.AllocTemp(SizeClass::INT);
    WithResultIn(size).Visit(expr.GetExprPtr());

    Mem array_mem = builder_.AllocArray(SizeClassFrom(expr.GetType().GetTypeId()), size);
    builder_.Mov(res_, array_mem);

    return VisitResult::SKIP;
  }

  VISIT_DECL(NewClassExpr, expr,) {
    Mem this_mem = builder_.AllocHeap(expr.GetTypeId());

    // TODO: Refactor with CallExpr.
    // Allocate argument temps and generate their code.
    vector<Mem> arg_mems;
    arg_mems.push_back(this_mem);
    for (int i = 0; i < expr.Args().Size(); ++i) {
      auto arg = expr.Args().At(i);
      Mem arg_mem = builder_.AllocTemp(SizeClassFrom(arg->GetTypeId()));
      WithResultIn(arg_mem).Visit(arg);
      arg_mems.push_back(arg_mem);
    }

    // Perform constructor call.
    {
      Mem tmp = builder_.AllocDummy();
      builder_.StaticCall(tmp, expr.GetTypeId().base, expr.GetMethodId(), arg_mems);
    }

    // Deallocate arg mems.
    while (!arg_mems.empty()) {
      arg_mems.pop_back();
    }

    builder_.Mov(res_, this_mem);

    return VisitResult::SKIP;
  }

  // Location result of the computation should be stored.
  Mem res_;
  bool lvalue_ = false;
  StreamBuilder& builder_;
  vector<ast::LocalVarId>& locals_;
  map<ast::LocalVarId, Mem>& locals_map_;
  TypeId tid_;
};

class ProgramIRGenerator final : public ast::Visitor {
 public:
  ProgramIRGenerator(const TypeInfoMap& tinfo_map) : tinfo_map_(tinfo_map) {}
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

  VISIT_DECL(TypeDecl, decl,) {
    if (decl.Kind() == TypeKind::INTERFACE) {
      return VisitResult::SKIP;
    }

    TypeId tid = decl.GetTypeId();
    Type type{tid.base, {}};

    // Only store fields with initialisers.
    vector<tuple<TypeId, FieldId, sptr<const Expr>>> fields;

    for (int i = 0; i < decl.Members().Size(); ++i) {
      sptr<const MemberDecl> member = decl.Members().At(i);
      auto meth = dynamic_pointer_cast<const MethodDecl, const MemberDecl>(member);
      if (meth != nullptr) {
        VisitMethodDeclImpl(meth, &type);
        continue;
      }

      auto field = dynamic_pointer_cast<const FieldDecl, const MemberDecl>(member);
      CHECK(field != nullptr);

      if (field->Mods().HasModifier(lexer::STATIC)) {
        // TODO: deal with static fields.
        continue;
      }

      if (field->ValPtr() == nullptr) {
        continue;
      }

      fields.push_back(make_tuple(field->GetType().GetTypeId(), field->GetFieldId(), field->ValPtr()));
    }

    {
      StreamBuilder builder;

      vector<Mem> mem_out;
      builder.AllocParams({SizeClass::PTR}, &mem_out);

      const TypeInfo& tinfo = tinfo_map_.LookupTypeInfo(tid);

      if (tinfo.extends.Size() > 0) {
        CHECK(tinfo.extends.Size() == 1);
        TypeId ptid = tinfo.extends.At(0);
        const TypeInfo& pinfo = tinfo_map_.LookupTypeInfo(ptid);
        MethodId mid = pinfo
          .methods
          .LookupMethod({true, pinfo.name, TypeIdList({})})
          .mid;

        Mem dummy = builder.AllocDummy();

        builder.StaticCall(dummy, ptid.base, mid, {mem_out[0]});
      }

      vector<ast::LocalVarId> empty_locals{kVarImplicitThis};
      map<ast::LocalVarId, Mem> locals_map{{kVarImplicitThis, mem_out[0]}};
      for (auto tup : fields) {
        Mem field = builder.AllocTemp(SizeClass::PTR);
        Mem val = builder.AllocTemp(SizeClassFrom(get<0>(tup)));

        builder.FieldAddr(field, mem_out[0], get<1>(tup), PosRange(-1, -1, -1));

        MethodIRGenerator gen(val, false, &builder, &empty_locals, &locals_map, tid);
        gen.Visit(get<2>(tup));

        builder.MovToAddr(field, val);
      }

      type.streams.push_back(builder.Build(false, tid.base, kInitMethodId));
    }

    current_unit_.types.push_back(type);

    return VisitResult::SKIP;
  }

  void VisitMethodDeclImpl(sptr<const MethodDecl> decl, Type* out) {
    StreamBuilder builder;

    vector<ast::LocalVarId> empty_locals;
    map<ast::LocalVarId, Mem> locals_map;
    bool is_entry_point = false;
    // TODO: don't hardcode to test and 16.
    if (decl->Name() == "test" || out->tid == 16) {
      Mem ret = builder.AllocDummy();

      // Entry point is a static method called "test" with no params.
      is_entry_point =
        (decl->Name() == "test"
         && decl->Mods().HasModifier(lexer::Modifier::STATIC)
         && decl->Params().Params().Size() == 0);

      MethodIRGenerator gen(ret, false, &builder, &empty_locals, &locals_map, {out->tid, 0});
      gen.Visit(decl);
    } else {
      // TODO: Dirty hack to get stdlib generating empty methods.
      vector<Mem> nothing;
      builder.AllocParams({}, &nothing);
    }
    // Return mem must be deallocated before Build is called.

    out->streams.push_back(builder.Build(is_entry_point, out->tid, decl->GetMethodId()));
  }

  ir::Program prog;

 private:
  ir::CompUnit current_unit_;
  const TypeInfoMap& tinfo_map_;
};

} // namespace

Program GenerateIR(sptr<const ast::Program> program, const TypeInfoMap& tinfo_map) {
  ProgramIRGenerator gen(tinfo_map);
  gen.Visit(program);
  return gen.prog;
}


} // namespace ir


