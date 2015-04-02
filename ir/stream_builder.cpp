#include "ir/stream_builder.h"

#include "ir/mem_impl.h"

using std::initializer_list;

using ast::FieldId;
using ast::MethodId;
using ast::TypeId;
using base::PosRange;
using types::StringId;

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

Mem StreamBuilder::PromoteToInt(Mem src) {
  CHECK(src.Size() != SizeClass::PTR);
  if (src.Size() == SizeClass::INT) {
    return src;
  }
  Mem tmp = AllocTemp(SizeClass::INT);
  Extend(tmp, src);
  return tmp;
}

Mem StreamBuilder::AllocHeap(TypeId tid) {
  CHECK(tid.ndims == 0);
  Mem tmp = AllocTemp(SizeClass::PTR);
  AppendOp(OpType::ALLOC_HEAP, {tmp.Id(), tid.base});
  SetAssigned({tmp});
  return tmp;
}

Mem StreamBuilder::AllocArray(SizeClass elemtype, Mem len) {
  AssertAssigned({len});
  Mem tmp = AllocTemp(SizeClass::PTR);
  AppendOp(OpType::ALLOC_ARRAY, {tmp.Id(), (u64)elemtype, len.Id()});
  SetAssigned({tmp});
  return tmp;
}


Mem StreamBuilder::AllocMem(SizeClass size, bool immutable) {
  CHECK(params_initialized_);

  MemId mid = next_mem_;
  ++next_mem_;

  auto iter_pair = unassigned_.insert(mid);
  CHECK(iter_pair.second);

  AppendOp(OpType::ALLOC_MEM, {mid, (u64)size, BoolArg(immutable)});

  auto impl = sptr<MemImpl>(new MemImpl{mid, size, this, immutable});
  return Mem(impl);
}

void StreamBuilder::DeallocMem(MemId mid) {
  // We don't emit DEALLOC_MEMs for parameters.
  if (mid <= params_.size()) {
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

void StreamBuilder::AllocParams(const vector<SizeClass>& sizes, vector<Mem>* out) {
  CHECK(!params_initialized_);
  params_initialized_ = true;

  for (SizeClass size : sizes) {
    MemId mid = next_mem_;
    ++next_mem_;

    auto impl = sptr<MemImpl>(new MemImpl{mid, size, this, false});
    out->push_back(Mem(impl));
  }

  params_ = sizes;

  CHECK(next_mem_ == (params_.size() + 1));
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

void StreamBuilder::ConstNumeric(Mem mem, i32 val) {
  Const(mem, (u64)(i64)val);
}

void StreamBuilder::ConstBool(Mem mem, bool b) {
  CHECK(mem.Size() == SizeClass::BOOL);
  Const(mem, b ? 1 : 0);
}

void StreamBuilder::ConstNull(Mem mem) {
  CHECK(mem.Size() == SizeClass::PTR);
  Const(mem, 0);
}

void StreamBuilder::ConstString(Mem dst, StringId id) {
  CHECK(dst.Size() == SizeClass::PTR);
  AppendOp(OpType::CONST_STR, {dst.Id(), id});
  SetAssigned({dst});
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

void StreamBuilder::FieldDeref(Mem dst, Mem src, TypeId::Base tid, FieldId fid, PosRange pos) {
  AssertAssigned({src});
  AppendOp(OpType::FIELD_DEREF, {dst.Id(), src.Id(), tid, fid, (u64)pos.begin});
  SetAssigned({dst});
}

void StreamBuilder::FieldAddr(Mem dst, Mem src, TypeId::Base tid, FieldId fid, PosRange pos) {
  AssertAssigned({src});
  AppendOp(OpType::FIELD_ADDR, {dst.Id(), src.Id(), tid, fid, (u64)pos.begin});
  SetAssigned({dst});
}

void StreamBuilder::ArrayDeref(Mem dst, Mem array, Mem index, SizeClass elemsize, PosRange pos) {
  AssertAssigned({array, index});
  AppendOp(OpType::ARRAY_DEREF, {dst.Id(), array.Id(), index.Id(), (u64)elemsize, (u64)pos.begin});
  SetAssigned({dst});
}

void StreamBuilder::ArrayAddr(Mem dst, Mem array, Mem index, SizeClass elemsize, PosRange pos) {
  AssertAssigned({array, index});
  AppendOp(OpType::ARRAY_ADDR, {dst.Id(), array.Id(), index.Id(), (u64)elemsize, (u64)pos.begin});
  SetAssigned({dst});
}

void StreamBuilder::BinOp(Mem dst, Mem lhs, Mem rhs, OpType op) {
  AssertAssigned({lhs, rhs});
  AppendOp(op, {dst.Id(), lhs.Id(), rhs.Id()});
  SetAssigned({dst});
}

void StreamBuilder::UnOp(Mem dst, Mem src, OpType op) {
  AssertAssigned({src});
  AppendOp(op, {dst.Id(), src.Id()});
  SetAssigned({dst});
}

void StreamBuilder::Add(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::ADD);
}

void StreamBuilder::Sub(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::SUB);
}

void StreamBuilder::Mul(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::MUL);
}

void StreamBuilder::Div(Mem dst, Mem lhs, Mem rhs, PosRange pos) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::DIV, {dst.Id(), lhs.Id(), rhs.Id(), (u64)pos.begin});
  SetAssigned({dst});
}

void StreamBuilder::Mod(Mem dst, Mem lhs, Mem rhs, PosRange pos) {
  AssertAssigned({lhs, rhs});
  AppendOp(OpType::MOD, {dst.Id(), lhs.Id(), rhs.Id(), (u64)pos.begin});
  SetAssigned({dst});
}

void StreamBuilder::Jmp(LabelId lid) {
  AppendOp(OpType::JMP, {lid});
}

void StreamBuilder::JmpIf(LabelId lid, Mem cond) {
  AssertAssigned({cond});
  AppendOp(OpType::JMP_IF, {lid, cond.Id()});
}

void StreamBuilder::Lt(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::LT);
}

void StreamBuilder::Leq(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::LEQ);
}

void StreamBuilder::Gt(Mem dst, Mem lhs, Mem rhs) {
  Lt(dst, rhs, lhs);
}

void StreamBuilder::Geq(Mem dst, Mem lhs, Mem rhs) {
  Leq(dst, rhs, lhs);
}

void StreamBuilder::Eq(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::EQ);
}

void StreamBuilder::Neq(Mem dst, Mem lhs, Mem rhs) {
  Mem tmp = AllocTemp(SizeClass::BOOL);
  Eq(tmp, lhs, rhs);
  Not(dst, tmp);
}

void StreamBuilder::Not(Mem dst, Mem src) {
  UnOp(dst, src, OpType::NOT);
}

void StreamBuilder::Neg(Mem dst, Mem src) {
  UnOp(dst, src, OpType::NEG);
}

void StreamBuilder::And(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::AND);
}

void StreamBuilder::Or(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::OR);
}

void StreamBuilder::Xor(Mem dst, Mem lhs, Mem rhs) {
  BinOp(dst, lhs, rhs, OpType::XOR);
}

void StreamBuilder::Extend(Mem dst, Mem src) {
  UnOp(dst, src, OpType::EXTEND);
}

void StreamBuilder::Truncate(Mem dst, Mem src) {
  UnOp(dst, src, OpType::TRUNCATE);
}

void StreamBuilder::StaticCall(Mem dst, TypeId::Base tid, MethodId mid, const vector<Mem>& args, PosRange pos) {
  size_t begin = args_.size();

  args_.push_back(dst.Id());
  args_.push_back(tid);
  args_.push_back(mid);
  args_.push_back(pos.begin);
  args_.push_back(args.size());
  for (auto val : args) {
    AssertAssigned({val});
    args_.push_back(val.Id());
  }

  ops_.push_back({OpType::STATIC_CALL, begin, args_.size()});
  if (dst.IsValid()) {
    SetAssigned({dst});
  }
}

void StreamBuilder::DynamicCall(Mem dst, Mem this_ptr, ast::MethodId mid, const vector<Mem>& args, PosRange pos) {
  AssertAssigned({this_ptr});
  size_t begin = args_.size();

  args_.push_back(dst.Id());
  args_.push_back(this_ptr.Id());
  args_.push_back(mid);
  args_.push_back(pos.begin);
  args_.push_back(args.size());
  for (auto val : args) {
    AssertAssigned({val});
    args_.push_back(val.Id());
  }

  ops_.push_back({OpType::DYNAMIC_CALL, begin, args_.size()});
  if (dst.IsValid()) {
    SetAssigned({dst});
  }
}

void StreamBuilder::GetTypeInfo(Mem dst, Mem src) {
  AssertAssigned({src});
  size_t begin = args_.size();
  args_.push_back(dst.Id());
  args_.push_back(src.Id());
  ops_.push_back({OpType::GET_TYPEINFO, begin, args_.size()});
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
  CHECK(params_initialized_);
  return Stream{is_entry_point, tid, mid, args_, ops_, params_};
}

} // namespace ir
