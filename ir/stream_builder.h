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

  // Allocate a label id; the Builder guarantees that the returned id will be
  // unique for this stream.
  LabelId AllocLabel();

  // Emit a label as the next instruction.
  void EmitLabel(LabelId);

  // Get a reference to a constant i32 value.
  Mem ConstInt32(i32);

  // Get a reference to a constant bool value.
  Mem ConstBool(bool);

  // Emit *dst = *src.
  void Mov(Mem dst, Mem src);

  // Emit *dst = src.
  void MovAddr(Mem dst, Mem src);

  // Emit *dst = *lhs + *rhs.
  void Add(Mem dst, Mem lhs, Mem rhs);

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

  Mem Const(SizeClass, u64);

  void AssertAssigned(const std::initializer_list<Mem>& mems) const;
  void SetAssigned(const std::initializer_list<Mem>& mems);

  set<u64> unassigned_;

  vector<u64> args_;
  vector<Op> ops_;

  MemId next_mem_ = 0;
  LabelId next_label_ = 0;
};

} // namespace ir

#endif
