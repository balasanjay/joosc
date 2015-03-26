#ifndef IR_STREAM_BUILDER_H
#define IR_STREAM_BUILDER_H

#include "ast/ids.h"
#include "ir/mem.h"
#include "ir/stream.h"

namespace ir {

// Forward declared to avoid circular dep.
struct MemImpl;

class StreamBuilder {
 public:
  Mem AllocTemp(SizeClass);

  Mem AllocLocal(SizeClass);

  Mem AllocDummy();

  void AllocParams(const vector<SizeClass>& sizes, vector<Mem>* out);

  // Allocate a label id; the Builder guarantees that the returned id will be
  // unique for this stream.
  LabelId AllocLabel();

  // Emit a label as the next instruction.
  void EmitLabel(LabelId);

  // Writes a constant i32 value to the given Mem.
  void ConstInt32(Mem, i32);

  // Writes a constant bool value to the given Mem.
  void ConstBool(Mem, bool);

  // Emit *dst = *src.
  void Mov(Mem dst, Mem src);

  // Emit *dst = src.
  void MovAddr(Mem dst, Mem src);

  // Emit **dst = *src.
  void MovToAddr(Mem dst, Mem src);

  // Emit *dst = *lhs + *rhs.
  void Add(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs - *rhs.
  void Sub(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs * *rhs.
  void Mul(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs / *rhs.
  void Div(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs % *rhs.
  void Mod(Mem dst, Mem lhs, Mem rhs);

  // Emit an unconditional jump to the label lid.
  // Building the Stream will validate that the referenced label exists.
  void Jmp(LabelId lid);

  // Emit a conditional jump. The SizeClass of the provided Mem must be a BOOL.
  void JmpIf(LabelId, Mem);

  // Emit *dst = *lhs < *rhs. dst must have SizeClass BOOL.
  void Lt(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs <= *rhs. dst must have SizeClass BOOL.
  void Leq(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs > *rhs. dst must have SizeClass BOOL.
  void Gt(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs >= *rhs. dst must have SizeClass BOOL.
  void Geq(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs == *rhs. dst must have SizeClass BOOL.
  void Eq(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs != *rhs. dst must have SizeClass BOOL.
  void Neq(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = !*src. dst and src must have SizeClass BOOL.
  void Not(Mem dst, Mem src);

  // Emit *dst = *lhs & *rhs. They must all have SizeClass BOOL.
  void And(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs | *rhs. They must all have SizeClass BOOL.
  void Or(Mem dst, Mem lhs, Mem rhs);

  // Emit *dst = *lhs ^ *rhs. They must all have SizeClass BOOL.
  void Xor(Mem dst, Mem lhs, Mem rhs);

  // Return with no value.
  void Ret();

  // Return with a value.
  void Ret(Mem);

  // Builds a stream of IR.
  Stream Build(bool is_entry_point, ast::TypeId::Base tid, ast::MethodId mid) const;

 private:
  friend struct MemImpl;

  void AppendOp(OpType type, const std::initializer_list<u64>& args);

  Mem AllocMem(SizeClass, bool);
  void DeallocMem(MemId);

  void Const(Mem, u64);
  void BinOp(Mem, Mem, Mem, OpType);

  void AssertAssigned(const std::initializer_list<Mem>& mems) const;
  void SetAssigned(const std::initializer_list<Mem>& mems);

  set<u64> unassigned_;

  vector<u64> args_;
  vector<Op> ops_;

  bool params_initialized_ = false;
  vector<SizeClass> params_;

  MemId next_mem_ = kFirstMemId;
  LabelId next_label_ = 0;
};

} // namespace ir

#endif
