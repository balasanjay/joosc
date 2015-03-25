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
  auto iter = unassigned_.find(mid);
  CHECK(iter == unassigned_.end());

  AppendOp(OpType::DEALLOC_MEM, {mid});
}

Mem StreamBuilder::AllocTemp(SizeClass size) {
  return AllocMem(size, BoolArg(true));
}

Mem StreamBuilder::AllocLocal(SizeClass size) {
  return AllocMem(size, BoolArg(false));
}

// Allocate a label id; the Builder guarantees that the returned id will be
// unique for this stream.
LabelId StreamBuilder::AllocLabel() {
  LabelId lid = next_label_;
  ++next_label_;
  return lid;
}

// Emit a label as the next instruction.
void StreamBuilder::EmitLabel(LabelId lid) {
  AppendOp(OpType::LABEL, {lid});
}

Mem StreamBuilder::Const(SizeClass size, u64 val) {
  Mem mem = AllocTemp(size);

  AppendOp(OpType::CONST, {mem.Id(), (u64)mem.Size(), val});
  SetAssigned({mem});

  return mem;
}

// Get a reference to a constant i32 value.
Mem StreamBuilder::ConstInt32(i32 val) {
  return Const(SizeClass::INT, (u64)(i64)val);
}

// Get a reference to a constant bool value.
Mem StreamBuilder::ConstBool(bool b) {
  return Const(SizeClass::BOOL, b ? 1 : 0);
}

// Emit *dst = *src.
void StreamBuilder::Mov(Mem dst, Mem src) {
  AssertAssigned({src});
  AppendOp(OpType::MOV, {dst.Id(), src.Id()});
  SetAssigned({dst});
}

// Emit *dst = src.
void StreamBuilder::MovAddr(Mem dst, Mem src) {
  AssertAssigned({src});
  AppendOp(OpType::MOV_ADDR, {dst.Id(), src.Id()});
  SetAssigned({dst});
}

// Emit *dst = *lhs + *rhs.
void StreamBuilder::Add(Mem dst, Mem lhs, Mem rhs) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::ADD, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

// Emit an unconditional jump to the label lid.
// Building the Stream will validate that the referenced label exists.
void StreamBuilder::Jmp(LabelId lid) {
  AppendOp(OpType::JMP, {lid});
}

// Emit a conditional jump. The SizeClass of the provided Mem must be a BOOL.
void StreamBuilder::JmpIf(LabelId lid, Mem cond) {
  AssertAssigned({cond});
  AppendOp(OpType::JMP_IF, {lid, cond.Id()});
}

// Emit *dst = *lhs < *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Lt(Mem dst, Mem lhs, Mem rhs) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::LT, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

// Emit *dst = *lhs <= *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Leq(Mem dst, Mem lhs, Mem rhs) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::LEQ, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

// Emit *dst = *lhs > *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Gt(Mem dst, Mem lhs, Mem rhs) {
  Lt(dst, rhs, lhs);
}

// Emit *dst = *lhs >= *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Geq(Mem dst, Mem lhs, Mem rhs) {
  Leq(dst, rhs, lhs);
}

// Emit *dst = *lhs == *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Eq(Mem dst, Mem lhs, Mem rhs) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::EQ, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

// Emit *dst = *lhs != *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Neq(Mem dst, Mem lhs, Mem rhs) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::NEQ, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

// Emit *dst = !*src. dst and src must have SizeClass BOOL.
void StreamBuilder::Not(Mem dst, Mem src) {
  AssertAssigned({src});
  AppendOp(OpType::NOT, {dst.Id(), src.Id()});
  SetAssigned({dst});
}

Stream StreamBuilder::Build(bool is_entry_point, ast::TypeId::Base tid, ast::MethodId mid) const {
  return Stream{is_entry_point, tid, mid, args_, ops_};
}

} // namespace ir
