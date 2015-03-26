#include "ir/stream_builder.h"

#include "ir/mem_impl.h"

using std::initializer_list;

namespace ir {

namespace {

u64 BoolArg(bool b) {
  return b ? 1 : 0;
}

} // namespace

void StreamBuilder::AppendOp(OpType type, const initializer_list<u64>& args) {
  size_t begin = args_.size();
  for (auto val : args) {
    args_.push_back(val);
  }
  ops_.push_back({type, begin, args_.size()});
}

void StreamBuilder::AssertAssigned(const std::initializer_list<Mem>& mems) const {
  for (const auto& mem : mems) {
    auto iter = unassigned_.find(mem.Id());
    CHECK(iter == unassigned_.end());
  }
}
void StreamBuilder::SetAssigned(const std::initializer_list<Mem>& mems) {
  for (const auto& mem : mems) {
    CHECK(mem.IsValid());
    size_t n = unassigned_.erase(mem.Id());
    bool already_written = (n == 0);
    CHECK(!(mem.Immutable() && already_written));
  }
}

Mem StreamBuilder::AllocMem(SizeClass size, bool immutable) {
  MemId mid = next_mem_;
  ++next_mem_;

  auto iter_pair = unassigned_.insert(mid);
  CHECK(iter_pair.second);

  AppendOp(OpType::ALLOC_MEM, {mid, (u64)size, BoolArg(immutable)});

  auto impl = sptr<MemImpl>(new MemImpl{mid, size, this, immutable});
  return Mem(impl);
}

void StreamBuilder::DeallocMem(MemId mid) {
  if (mid == kInvalidMemId) {
    return;
  }
  auto iter = unassigned_.find(mid);
  CHECK(iter == unassigned_.end());

  AppendOp(OpType::DEALLOC_MEM, {mid});
}

Mem StreamBuilder::AllocTemp(SizeClass size) {
  return AllocMem(size, BoolArg(true));
}

Mem StreamBuilder::AllocDummy() {
  auto impl = sptr<MemImpl>(new MemImpl{kInvalidMemId, SizeClass::BOOL, this, true});
  return Mem(impl);
}

Mem StreamBuilder::AllocLocal(SizeClass size) {
  return AllocMem(size, BoolArg(false));
}

LabelId StreamBuilder::AllocLabel() {
  LabelId lid = next_label_;
  ++next_label_;
  return lid;
}

void StreamBuilder::EmitLabel(LabelId lid) {
  AppendOp(OpType::LABEL, {lid});
}

void StreamBuilder::Const(Mem mem, u64 val) {
  AppendOp(OpType::CONST, {mem.Id(), (u64)mem.Size(), val});
  SetAssigned({mem});
}

void StreamBuilder::ConstInt32(Mem mem, i32 val) {
  CHECK(mem.Size() == SizeClass::INT);
  Const(mem, (u64)(i64)val);
}

void StreamBuilder::ConstBool(Mem mem, bool b) {
  CHECK(mem.Size() == SizeClass::BOOL);
  Const(mem, b ? 1 : 0);
}

void StreamBuilder::Mov(Mem dst, Mem src) {
  AssertAssigned({src});
  AppendOp(OpType::MOV, {dst.Id(), src.Id()});
  SetAssigned({dst});
}

void StreamBuilder::MovAddr(Mem dst, Mem src) {
  AssertAssigned({src});
  AppendOp(OpType::MOV_ADDR, {dst.Id(), src.Id()});
  SetAssigned({dst});
}

void StreamBuilder::MovToAddr(Mem dst, Mem src) {
  AssertAssigned({src});
  AppendOp(OpType::MOV_TO_ADDR, {dst.Id(), src.Id()});
  // TODO: We're not sure exactly how to represent what's assigned.
}

void StreamBuilder::Arithmetic(Mem dst, Mem lhs, Mem rhs, OpType op) {
  AssertAssigned({lhs, rhs});
  AppendOp(op, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

void StreamBuilder::Add(Mem dst, Mem lhs, Mem rhs) {
  Arithmetic(dst, lhs, rhs, OpType::ADD);
}

void StreamBuilder::Sub(Mem dst, Mem lhs, Mem rhs) {
  Arithmetic(dst, lhs, rhs, OpType::SUB);
}

void StreamBuilder::Mul(Mem dst, Mem lhs, Mem rhs) {
  Arithmetic(dst, lhs, rhs, OpType::MUL);
}

void StreamBuilder::Div(Mem dst, Mem lhs, Mem rhs) {
  Arithmetic(dst, lhs, rhs, OpType::DIV);
}

void StreamBuilder::Mod(Mem dst, Mem lhs, Mem rhs) {
  Arithmetic(dst, lhs, rhs, OpType::MOD);
}

void StreamBuilder::Jmp(LabelId lid) {
  AppendOp(OpType::JMP, {lid});
}

void StreamBuilder::JmpIf(LabelId lid, Mem cond) {
  AssertAssigned({cond});
  AppendOp(OpType::JMP_IF, {lid, cond.Id()});
}

void StreamBuilder::Lt(Mem dst, Mem lhs, Mem rhs) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::LT, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

void StreamBuilder::Leq(Mem dst, Mem lhs, Mem rhs) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::LEQ, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

void StreamBuilder::Gt(Mem dst, Mem lhs, Mem rhs) {
  Lt(dst, rhs, lhs);
}

void StreamBuilder::Geq(Mem dst, Mem lhs, Mem rhs) {
  Leq(dst, rhs, lhs);
}

void StreamBuilder::Eq(Mem dst, Mem lhs, Mem rhs) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::EQ, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

void StreamBuilder::Neq(Mem dst, Mem lhs, Mem rhs) {
  Mem tmp = AllocTemp(SizeClass::BOOL);
  Eq(tmp, lhs, rhs);
  Not(dst, tmp);
}

void StreamBuilder::Not(Mem dst, Mem src) {
  AssertAssigned({src});
  AppendOp(OpType::NOT, {dst.Id(), src.Id()});
  SetAssigned({dst});
}

void StreamBuilder::Ret() {
  AppendOp(OpType::RET, {});
}

void StreamBuilder::Ret(Mem ret) {
  AssertAssigned({ret});
  AppendOp(OpType::RET, {ret.Id()});
}

Stream StreamBuilder::Build(bool is_entry_point, ast::TypeId::Base tid, ast::MethodId mid) const {
  return Stream{is_entry_point, tid, mid, args_, ops_};
}

} // namespace ir
