#include "ir/ir_generator.h"

#include <algorithm>
#include <tuple>

#include "ast/print_visitor.h"
#include "ast/ast.h"
#include "ast/extent.h"
#include "ast/visitor.h"
#include "ir/size.h"
#include "ir/stream_builder.h"
#include "lexer/lexer.h"
#include "types/type_info_map.h"
#include "types/typechecker.h"

using std::dynamic_pointer_cast;
using std::get;
using std::tuple;

using ast::ArrayIndexExpr;
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
using ast::kInstanceInitMethodId;
using ast::kStaticInitMethodId;
using ast::kTypeInitMethodId;
using ast::kVarImplicitThis;
using base::PosRange;
using types::ConstStringMap;
using types::TypeChecker;
using types::TypeIdList;
using types::TypeInfo;
using types::TypeInfoMap;
using types::TypeSet;

namespace ir {

namespace {

class MethodIRGenerator final : public ast::Visitor {
 public:
  MethodIRGenerator(Mem res, Mem array_rvalue, bool lvalue, StreamBuilder* builder, vector<ast::LocalVarId>* locals, map<ast::LocalVarId, Mem>* locals_map, TypeId tid, const ConstStringMap& string_map, const RuntimeLinkIds& rt_ids): res_(res), array_rvalue_(array_rvalue), lvalue_(lvalue), builder_(*builder), locals_(*locals), locals_map_(*locals_map), tid_(tid), string_map_(string_map), rt_ids_(rt_ids) {}

  MethodIRGenerator WithResultIn(Mem res, bool lvalue = false) {
    return WithResultIn(res, builder_.AllocDummy(), lvalue);
  }

  MethodIRGenerator WithResultIn(Mem res, Mem array_rvalue) {
    return WithResultIn(res, array_rvalue, true);
  }

  MethodIRGenerator WithResultIn(Mem res, Mem array_rvalue, bool lvalue) {
    return MethodIRGenerator(res, array_rvalue, lvalue, &builder_, &locals_, &locals_map_, tid_, string_map_, rt_ids_);
  }

  MethodIRGenerator WithLocals(vector<ast::LocalVarId>& locals) {
    return MethodIRGenerator(res_, array_rvalue_, lvalue_, &builder_, &locals, &locals_map_, tid_, string_map_, rt_ids_);
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
      builder_.StaticCall(res_, tid_.base, kInstanceInitMethodId, {param_mems[0]}, decl.NameToken().pos);
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

  VISIT_DECL(CastExpr, expr,) {
    TypeId from = expr.GetExpr().GetTypeId();
    TypeId to = expr.GetTypeId();

    SizeClass fromsize = SizeClassFrom(from);

    if (from == to) {
      return VisitResult::RECURSE;
    }

    Mem expr_mem = builder_.AllocTemp(fromsize);
    WithResultIn(expr_mem).Visit(expr.GetExprPtr());

    if (TypeChecker::IsReference(from) || TypeChecker::IsReference(to)) {
      LabelId short_circuit = builder_.AllocLabel();

      // If expr is null, jump past instanceof check.
      {
        Mem null_mem = builder_.AllocTemp(SizeClass::PTR);
        builder_.ConstNull(null_mem);

        Mem is_null = builder_.AllocTemp(SizeClass::BOOL);
        builder_.Eq(is_null, null_mem, expr_mem);
        builder_.JmpIf(short_circuit, is_null);
      }

      // Perform the instanceof check.
      {
        Mem instanceof = builder_.AllocTemp(SizeClass::BOOL);
        builder_.InstanceOf(instanceof, expr_mem, expr.GetType().GetTypeId(), expr.GetExpr().GetTypeId());
        builder_.CastExceptionIfFalse(instanceof, expr.Lparen().pos);
      }

      builder_.EmitLabel(short_circuit);
      builder_.Mov(res_, expr_mem);

      return VisitResult::SKIP;
    }

    if (TypeChecker::IsPrimitiveWidening(to, from)) {
      builder_.Extend(res_, expr_mem);
    } else {
      builder_.Truncate(res_, expr_mem);
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
    TypeId lhs_tid = expr.Lhs().GetTypeId();
    TypeId rhs_tid = expr.Rhs().GetTypeId();
    SizeClass lhs_size = SizeClassFrom(lhs_tid);
    SizeClass rhs_size = SizeClassFrom(rhs_tid);

    // Special code for short-circuiting boolean and or.
    if (expr.Op().type == lexer::AND) {
      Mem lhs = builder_.AllocLocal(lhs_size);
      Mem rhs = builder_.AllocTemp(rhs_size);
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
      Mem lhs = builder_.AllocLocal(lhs_size);
      Mem rhs = builder_.AllocTemp(rhs_size);
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

    Mem lhs_old = builder_.AllocTemp(is_assg ? SizeClass::PTR : lhs_size);
    Mem rhs_old = builder_.AllocTemp(rhs_size);

    Mem lhs_rvalue = builder_.AllocDummy();
    if (dynamic_cast<const ArrayIndexExpr*>(expr.LhsPtr().get()) != nullptr && !TypeChecker::IsPrimitive(expr.GetTypeId())) {
      lhs_rvalue = builder_.AllocTemp(SizeClass::PTR);
    }

    Mem lhs = lhs_old;
    Mem rhs = rhs_old;

    if (is_assg) {
      WithResultIn(lhs, lhs_rvalue).Visit(expr.LhsPtr());
    } else {
      WithResultIn(lhs).Visit(expr.LhsPtr());
    }

    WithResultIn(rhs).Visit(expr.RhsPtr());

    if (is_assg) {
      // If this was an array, check if the element type matches.
      if (lhs_rvalue.IsValid()) {
        builder_.CheckArrayStore(lhs_rvalue, rhs, expr.Op().pos);
      }

      builder_.MovToAddr(lhs, rhs);

      // The result of an assignment expression is the rhs. We don't bother
      // with this if it's in a top-level context.
      if (res_.IsValid()) {
        builder_.Mov(res_, rhs);
      }
      return VisitResult::SKIP;
    }

    // If we are adding strings.
    if (expr.GetTypeId() == rt_ids_.string_tid) {
      CHECK(expr.Op().type == lexer::ADD);

      Mem lhs_str = builder_.AllocTemp(SizeClass::PTR);
      if (lhs.Size() == SizeClass::PTR) {
        builder_.StaticCall(lhs_str, rt_ids_.stringops_type.base, rt_ids_.stringops_str, {lhs}, expr.Op().pos);
      } else {
        CHECK(types::TypeChecker::IsPrimitive(lhs_tid));
        builder_.StaticCall(lhs_str, rt_ids_.string_tid.base, rt_ids_.string_valueof.at(lhs_tid.base), {lhs}, expr.Op().pos);
      }

      Mem rhs_str = builder_.AllocTemp(SizeClass::PTR);
      if (rhs.Size() == SizeClass::PTR) {
        builder_.StaticCall(rhs_str, rt_ids_.stringops_type.base, rt_ids_.stringops_str, {rhs}, expr.Op().pos);
      } else {
        CHECK(types::TypeChecker::IsPrimitive(rhs_tid));
        builder_.StaticCall(rhs_str, rt_ids_.string_tid.base, rt_ids_.string_valueof.at(rhs_tid.base), {rhs}, expr.Op().pos);
      }

      builder_.DynamicCall(res_, lhs_str, rt_ids_.string_concat, {rhs_str}, expr.Op().pos);

      return VisitResult::SKIP;
    }

    // Only perform binary numeric promotion if we are performing operations on
    // numeric types.
    if (lhs.Size() != SizeClass::PTR && lhs.Size() != SizeClass::BOOL) {
      lhs = builder_.PromoteToInt(lhs);
    }
    if (rhs.Size() != SizeClass::PTR && rhs.Size() != SizeClass::BOOL) {
      rhs = builder_.PromoteToInt(rhs);
    }

#define C(fn) builder_.fn(res_, lhs, rhs); break;
    switch (expr.Op().type) {
      case lexer::ADD:  C(Add);
      case lexer::SUB:  C(Sub);
      case lexer::MUL:  C(Mul);
      case lexer::DIV:  builder_.Div(res_, lhs, rhs, expr.Op().pos); break;
      case lexer::MOD:  builder_.Mod(res_, lhs, rhs, expr.Op().pos); break;
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
    builder_.ConstNumeric(res_, expr.Value());
    return VisitResult::SKIP;
  }

  VISIT_DECL(CharLitExpr, expr,) {
    builder_.ConstNumeric(res_, expr.Char());
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

  VISIT_DECL(StringLitExpr, expr,) {
    builder_.ConstString(res_, string_map_.at(expr.Str()));
    return VisitResult::SKIP;
  }

  VISIT_DECL(ThisExpr,,) {
    auto i = locals_map_.find(kVarImplicitThis);
    CHECK(i != locals_map_.end());
    builder_.Mov(res_, i->second);
    return VisitResult::SKIP;
  }

  VISIT_DECL(FieldDerefExpr, expr,) {
    bool is_static = false;
    TypeId::Base base_tid = expr.Base().GetTypeId().base;
    {
      auto static_base = dynamic_cast<const StaticRefExpr*>(expr.BasePtr().get());
      if (static_base != nullptr) {
        is_static = true;
        base_tid = static_base->GetRefType().GetTypeId().base;
      }
    }

    Mem tmp = builder_.AllocDummy();

    if (!is_static) {
      tmp = builder_.AllocTemp(SizeClass::PTR);
      // We want an rvalue of the pointer, so set lvalue to false.
      WithResultIn(tmp, false).Visit(expr.BasePtr());
    }

    if (lvalue_) {
      builder_.FieldAddr(res_, tmp, base_tid, expr.GetFieldId(), expr.GetToken().pos);
    } else {
      builder_.FieldDeref(res_, tmp, base_tid, expr.GetFieldId(), expr.GetToken().pos);
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
      if (array_rvalue_.IsValid()) {
        builder_.Mov(array_rvalue_, array);
      }

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
    auto static_base = dynamic_cast<const StaticRefExpr*>(expr.BasePtr().get());
    Mem this_ptr = builder_.AllocDummy();
    if (static_base == nullptr) {
      this_ptr = builder_.AllocTemp(SizeClass::PTR);
      WithResultIn(this_ptr).Visit(expr.BasePtr());
    }

    // Allocate argument temps and generate their code.
    vector<Mem> arg_mems;
    for (int i = 0; i < expr.Args().Size(); ++i) {
      auto arg = expr.Args().At(i);
      Mem arg_mem = builder_.AllocTemp(SizeClassFrom(arg->GetTypeId()));
      WithResultIn(arg_mem).Visit(arg);
      arg_mems.push_back(arg_mem);
    }

    if (static_base == nullptr) {
      builder_.DynamicCall(res_, this_ptr, expr.GetMethodId(), arg_mems, expr.Lparen().pos);
    } else {
      TypeId tid = static_base->GetRefType().GetTypeId();
      builder_.StaticCall(res_, tid.base, expr.GetMethodId(), arg_mems, expr.Lparen().pos);
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

    Mem array_mem = builder_.AllocArray(expr.GetType().GetTypeId(), size, expr.NewToken().pos);
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
      builder_.StaticCall(tmp, expr.GetTypeId().base, expr.GetMethodId(), arg_mems, expr.Lparen().pos);
    }

    // Deallocate arg mems.
    while (!arg_mems.empty()) {
      arg_mems.pop_back();
    }

    // New-expressions can be at top-level, so we might not have a result value
    // to write to.
    if (res_.Id() != kInvalidMemId) {
      builder_.Mov(res_, this_mem);
    }

    return VisitResult::SKIP;
  }

  VISIT_DECL(InstanceOfExpr, expr,) {
    Mem lhs = builder_.AllocTemp(SizeClass::PTR);
    WithResultIn(lhs, false).Visit(expr.LhsPtr());

    Mem tmp = builder_.AllocLocal(SizeClass::BOOL);

    // If lhs is null, then we short-circuit the INSTANCE_OF operation, and
    // just immediately return false.
    LabelId short_circuit = builder_.AllocLabel();
    {
      builder_.ConstBool(tmp, false);

      Mem null_mem = builder_.AllocTemp(SizeClass::PTR);
      builder_.ConstNull(null_mem);

      Mem is_null = builder_.AllocTemp(SizeClass::BOOL);
      builder_.Eq(is_null, null_mem, lhs);

      builder_.JmpIf(short_circuit, is_null);
    }

    builder_.InstanceOf(tmp, lhs, expr.GetType().GetTypeId(), expr.Lhs().GetTypeId());

    builder_.EmitLabel(short_circuit);
    builder_.Mov(res_, tmp);

    return VisitResult::SKIP;
  }

  // Location result of the computation should be stored.
  Mem res_;
  Mem array_rvalue_;
  bool lvalue_ = false;
  StreamBuilder& builder_;
  vector<ast::LocalVarId>& locals_;
  map<ast::LocalVarId, Mem>& locals_map_;
  TypeId tid_;
  const ConstStringMap& string_map_;
  const RuntimeLinkIds& rt_ids_;
};

class ProgramIRGenerator final : public ast::Visitor {
 public:
  ProgramIRGenerator(const TypeInfoMap& tinfo_map, const ConstStringMap& string_map, const RuntimeLinkIds& rt_ids) : tinfo_map_(tinfo_map), string_map_(string_map), rt_ids_(rt_ids) {}
  VISIT_DECL(CompUnit, unit, ) {
    stringstream ss;
    ss << 'f' << unit.FileId() << ".s";
    current_unit_.filename = ss.str();
    current_unit_.fileid = unit.FileId();

    for (int i = 0; i < unit.Types().Size(); ++i) {
      Visit(unit.Types().At(i));
    }
    prog.units.push_back(current_unit_);
    current_unit_ = ir::CompUnit();
    return VisitResult::SKIP;
  }

  VISIT_DECL(TypeDecl, decl,) {
    TypeId tid = decl.GetTypeId();
    Type type{tid.base, {}};
    const TypeInfo& tinfo = tinfo_map_.LookupTypeInfo(tid);

    // Runtime type info initialization.
    {
      u32 num_parents = tinfo.extends.Size() + tinfo.implements.Size();
      StreamBuilder t_builder;
      {
        vector<Mem> mem_out;
        t_builder.AllocParams({}, &mem_out);
      }

      {
        Mem size = t_builder.AllocTemp(SizeClass::INT);
        t_builder.ConstNumeric(size, num_parents);
        {
          Mem array = t_builder.AllocArray(rt_ids_.type_info_tid, size, base::PosRange(0, 0, 0));
          auto write_parent = [&](i32 i, ast::TypeId::Base p_tid) {
            // Get parent pointer from parent type's static field.
            // Guaranteed to be filled because of static type
            // initialization being done in topsort order.
            Mem parent = t_builder.AllocTemp(SizeClass::PTR);
            {
              Mem dummy = t_builder.AllocDummy();
              t_builder.FieldDeref(parent, dummy, p_tid, ast::kStaticTypeInfoId, base::PosRange(0, 0, 0));
            }
            Mem idx = t_builder.AllocTemp(SizeClass::INT);
            t_builder.ConstNumeric(idx, i);

            Mem array_slot = t_builder.AllocLocal(SizeClass::PTR);
            t_builder.ArrayAddr(array_slot, array, idx, SizeClass::PTR, base::PosRange(0, 0, 0));
            t_builder.MovToAddr(array_slot, parent);
          };

          i32 parent_idx = 0;
          for (i32 i = 0; i < tinfo.extends.Size(); ++i) {
            write_parent(parent_idx, tinfo.extends.At(i).base);
            ++parent_idx;
          }
          for (i32 i = 0; i < tinfo.implements.Size(); ++i) {
            write_parent(parent_idx, tinfo.implements.At(i).base);
            ++parent_idx;
          }

          // Construct the TypeInfo.
          {
            Mem rt_type_info = t_builder.AllocHeap(rt_ids_.type_info_tid);

            vector<Mem> arg_mems;
            arg_mems.push_back(rt_type_info);
            {
              Mem tid_mem = t_builder.AllocTemp(SizeClass::INT);
              t_builder.ConstNumeric(tid_mem, tid.base);
              arg_mems.push_back(tid_mem);
            }
            arg_mems.push_back(array);

            // Perform constructor call.
            {
              Mem tmp = t_builder.AllocDummy();
              t_builder.StaticCall(tmp, rt_ids_.type_info_tid.base, rt_ids_.type_info_constructor, arg_mems, decl.NameToken().pos);
            }

            // Write the TypeInfo to the special static field on this class.
            {
              Mem field = t_builder.AllocTemp(SizeClass::PTR);
              {
                Mem dummy_src = t_builder.AllocDummy();
                t_builder.FieldAddr(field, dummy_src, tid.base, ast::kStaticTypeInfoId, base::PosRange(0, 0, 0));
              }
              t_builder.MovToAddr(field, rt_type_info);
            }
          }
        }
      }
      type.streams.push_back(t_builder.Build(false, tid.base, kTypeInitMethodId));
    }

    if (decl.Kind() == TypeKind::INTERFACE) {
      current_unit_.types.push_back(type);
      return VisitResult::SKIP;
    }

    // Only store fields with initialisers.
    vector<sptr<const FieldDecl>> fields;

    for (int i = 0; i < decl.Members().Size(); ++i) {
      sptr<const MemberDecl> member = decl.Members().At(i);
      auto meth = dynamic_pointer_cast<const MethodDecl, const MemberDecl>(member);
      if (meth != nullptr) {
        VisitMethodDeclImpl(meth, &type);
        continue;
      }

      auto field = dynamic_pointer_cast<const FieldDecl, const MemberDecl>(member);
      CHECK(field != nullptr);

      if (field->ValPtr() == nullptr) {
        continue;
      }

      fields.push_back(field);
    }

    {
      StreamBuilder i_builder;
      StreamBuilder s_builder;

      // Get the `this' ptr.
      Mem i_this_ptr = i_builder.AllocDummy();
      {
        vector<Mem> mem_out;
        i_builder.AllocParams({SizeClass::PTR}, &mem_out);
        i_this_ptr = mem_out.at(0);
      }

      {
        vector<Mem> mem_out;
        s_builder.AllocParams({}, &mem_out);
      }


      if (tinfo.extends.Size() > 0) {
        CHECK(tinfo.extends.Size() == 1);
        TypeId ptid = tinfo.extends.At(0);
        const TypeInfo& pinfo = tinfo_map_.LookupTypeInfo(ptid);
        MethodId mid = pinfo
          .methods
          .LookupMethod({true, pinfo.name, TypeIdList({})})
          .mid;

        Mem dummy = i_builder.AllocDummy();

        i_builder.StaticCall(dummy, ptid.base, mid, {i_this_ptr}, decl.NameToken().pos);
      }

      for (auto field : fields) {
        StreamBuilder* builder = nullptr;
        vector<ast::LocalVarId> empty_locals;
        map<ast::LocalVarId, Mem> locals_map;
        Mem this_ptr = i_this_ptr;

        if (field->Mods().HasModifier(lexer::STATIC)) {
          builder = &s_builder;
          this_ptr = s_builder.AllocDummy();
        } else {
          builder = &i_builder;
          empty_locals.push_back(kVarImplicitThis);
          locals_map.insert({kVarImplicitThis, i_this_ptr});
        }

        Mem f_mem = builder->AllocTemp(SizeClass::PTR);
        Mem val = builder->AllocTemp(SizeClassFrom(field->GetType().GetTypeId()));

        builder->FieldAddr(f_mem, this_ptr, tid.base, field->GetFieldId(), PosRange(0, 0, 0));

        MethodIRGenerator gen(val, builder->AllocDummy(), false, builder, &empty_locals, &locals_map, tid, string_map_, rt_ids_);
        gen.Visit(field->ValPtr());

        builder->MovToAddr(f_mem, val);
      }

      type.streams.push_back(i_builder.Build(false, tid.base, kInstanceInitMethodId));
      type.streams.push_back(s_builder.Build(false, tid.base, kStaticInitMethodId));
    }

    current_unit_.types.push_back(type);

    return VisitResult::SKIP;
  }

  void VisitMethodDeclImpl(sptr<const MethodDecl> decl, Type* out) {
    StreamBuilder builder;

    vector<ast::LocalVarId> empty_locals;
    map<ast::LocalVarId, Mem> locals_map;
    bool is_entry_point = false;
    {
      Mem ret = builder.AllocDummy();

      // Entry point is a static method called "test" with no params.
      is_entry_point =
        (decl->Name() == "test"
         && decl->Mods().HasModifier(lexer::Modifier::STATIC)
         && decl->Params().Params().Size() == 0);

      MethodIRGenerator gen(ret, builder.AllocDummy(), false, &builder, &empty_locals, &locals_map, {out->tid, 0}, string_map_, rt_ids_);
      gen.Visit(decl);
    }


    // Return mem must be deallocated before Build is called.
    out->streams.push_back(builder.Build(is_entry_point, out->tid, decl->GetMethodId()));
  }

  ir::Program prog;

 private:
  CompUnit current_unit_;
  const TypeInfoMap& tinfo_map_;
  const ConstStringMap& string_map_;
  const RuntimeLinkIds& rt_ids_;
};

RuntimeLinkIds LookupRuntimeIds(const TypeSet& typeset, const TypeInfoMap& tinfo_map) {
  RuntimeLinkIds rt_ids;
  base::ErrorList throwaway;

  rt_ids.object_tid = typeset.TryGet("java.lang.Object");
  CHECK(rt_ids.object_tid.IsValid());

  rt_ids.string_tid = typeset.TryGet("java.lang.String");
  CHECK(rt_ids.string_tid.IsValid());
  TypeInfo string_tinfo = tinfo_map.LookupTypeInfo(rt_ids.string_tid);

  rt_ids.string_concat = string_tinfo.methods.ResolveCall(
      tinfo_map,
      rt_ids.string_tid,
      types::CallContext::INSTANCE,
      rt_ids.string_tid,
      TypeIdList({rt_ids.string_tid}),
      "concat", base::PosRange(-1, -1, -1), &throwaway);
  CHECK(!throwaway.IsFatal());
  CHECK(rt_ids.string_concat != ast::kErrorMethodId);

  auto valueof_method = [&](ast::TypeId tid) {
    ast::MethodId mid = string_tinfo.methods.ResolveCall(
        tinfo_map,
        rt_ids.string_tid,
        types::CallContext::STATIC,
        rt_ids.string_tid,
        TypeIdList({tid}),
        "valueOf", base::PosRange(-1, -1, -1), &throwaway);
    CHECK(!throwaway.IsFatal());
    CHECK(mid != ast::kErrorMethodId);
    rt_ids.string_valueof.insert({tid.base, mid});
  };

  valueof_method(ast::TypeId::kInt);
  valueof_method(ast::TypeId::kShort);
  valueof_method(ast::TypeId::kChar);
  valueof_method(ast::TypeId::kByte);
  valueof_method(ast::TypeId::kBool);

  rt_ids.type_info_tid = typeset.TryGet("__joos_internal__.TypeInfo");
  CHECK(rt_ids.type_info_tid.IsValid());
  TypeInfo type_info_tinfo = tinfo_map.LookupTypeInfo(rt_ids.type_info_tid);

  rt_ids.type_info_constructor = type_info_tinfo.methods.ResolveCall(
      tinfo_map,
      rt_ids.type_info_tid,
      types::CallContext::CONSTRUCTOR,
      rt_ids.type_info_tid,
      TypeIdList({TypeId::kInt, {rt_ids.type_info_tid.base, 1}}),
      "TypeInfo", base::PosRange(-1, -1, -1), &throwaway);
  CHECK(!throwaway.IsFatal());
  CHECK(rt_ids.type_info_constructor != ast::kErrorMethodId);

  rt_ids.type_info_instanceof = type_info_tinfo.methods.ResolveCall(
      tinfo_map,
      rt_ids.type_info_tid,
      types::CallContext::STATIC,
      rt_ids.type_info_tid,
      TypeIdList({rt_ids.type_info_tid, rt_ids.type_info_tid}),
      "InstanceOf", base::PosRange(-1, -1, -1), &throwaway);
  CHECK(!throwaway.IsFatal());
  CHECK(rt_ids.type_info_instanceof != ast::kErrorMethodId);

  rt_ids.type_info_num_types = type_info_tinfo.fields.ResolveAccess(
      tinfo_map,
      rt_ids.type_info_tid,
      types::CallContext::STATIC,
      rt_ids.type_info_tid,
      "num_types",
      base::PosRange(-1, -1, -1),
      &throwaway);
  CHECK(!throwaway.IsFatal());
  CHECK(rt_ids.type_info_num_types != ast::kErrorFieldId);

  rt_ids.stringops_type = typeset.TryGet("__joos_internal__.StringOps");
  CHECK(rt_ids.stringops_type.IsValid());

  TypeInfo stringops_tinfo = tinfo_map.LookupTypeInfo(rt_ids.stringops_type);
  rt_ids.stringops_str = stringops_tinfo.methods.ResolveCall(
      tinfo_map,
      rt_ids.stringops_type,
      types::CallContext::STATIC,
      rt_ids.stringops_type,
      TypeIdList({rt_ids.object_tid}),
      "Str", base::PosRange(-1, -1, -1), &throwaway);
  CHECK(!throwaway.IsFatal());
  CHECK(rt_ids.stringops_str != ast::kErrorMethodId);

  rt_ids.stackframe_type = typeset.TryGet("__joos_internal__.StackFrame");
  CHECK(rt_ids.type_info_tid.IsValid());
  TypeInfo stackframe_tinfo = tinfo_map.LookupTypeInfo(rt_ids.stackframe_type);
  rt_ids.stackframe_print = stackframe_tinfo.methods.ResolveCall(
      tinfo_map,
      rt_ids.stackframe_type,
      types::CallContext::INSTANCE,
      rt_ids.stackframe_type,
      TypeIdList({}),
      "Print", base::PosRange(-1, -1, -1), &throwaway);
  rt_ids.stackframe_print_ex = stackframe_tinfo.methods.ResolveCall(
      tinfo_map,
      rt_ids.stackframe_type,
      types::CallContext::STATIC,
      rt_ids.stackframe_type,
      TypeIdList({TypeId::kInt}),
      "PrintException", base::PosRange(-1, -1, -1), &throwaway);
  CHECK(!throwaway.IsFatal());
  CHECK(rt_ids.stackframe_print != ast::kErrorMethodId);
  CHECK(rt_ids.stackframe_print_ex != ast::kErrorMethodId);

  rt_ids.array_runtime_type = typeset.TryGet("__joos_internal__.Array");
  CHECK(rt_ids.array_runtime_type.IsValid());

  return rt_ids;
}

} // namespace

Program GenerateIR(sptr<const ast::Program> program, const TypeSet& typeset, const TypeInfoMap& tinfo_map, const ConstStringMap& string_map) {
  RuntimeLinkIds rt_ids = LookupRuntimeIds(typeset, tinfo_map);
  ProgramIRGenerator gen(tinfo_map, string_map, rt_ids);
  gen.Visit(program);
  gen.prog.rt_ids = rt_ids;
  return gen.prog;
}


} // namespace ir


