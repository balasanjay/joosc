#include "ir/stream_builder.h"

namespace ir {

Mem StreamBuilder::AllocTemp(SizeClass) {
  UNIMPLEMENTED();
}

Mem StreamBuilder::AllocLocal(SizeClass) {
  UNIMPLEMENTED();
}

// Allocate a label id; the Builder guarantees that the returned id will be
// unique for this stream.
LabelId StreamBuilder::AllocLabel() {
  UNIMPLEMENTED();
}

// Emit a label as the next instruction.
void StreamBuilder::EmitLabel(LabelId) {
  UNIMPLEMENTED();
}

// Get a reference to a constant i32 value.
Mem StreamBuilder::ConstInt32(i32) {
  UNIMPLEMENTED();
}

// Get a reference to a constant bool value.
Mem StreamBuilder::ConstBool(bool) {
  UNIMPLEMENTED();
}

// Emit *dst = *src.
void StreamBuilder::Mov(Mem, Mem) {
  UNIMPLEMENTED();
}

// Emit *dst = src.
void StreamBuilder::MovAddr(Mem, Mem) {
  UNIMPLEMENTED();
}

// Emit *dst = *lhs + *rhs.
void StreamBuilder::Add(Mem, Mem, Mem) {
  UNIMPLEMENTED();
}

// Emit an unconditional jump to the label lid.
// Building the Stream will validate that the referenced label exists.
void StreamBuilder::Jmp(LabelId) {
  UNIMPLEMENTED();
}

// Emit a conditional jump. The SizeClass of the provided Mem must be a BOOL.
void StreamBuilder::JmpIf(LabelId, Mem) {
  UNIMPLEMENTED();
}

// Emit *dst = *lhs < *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Lt(Mem, Mem, Mem) {
  UNIMPLEMENTED();
}

// Emit *dst = *lhs <= *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Leq(Mem, Mem, Mem) {
  UNIMPLEMENTED();
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
void StreamBuilder::Eq(Mem, Mem, Mem) {
  UNIMPLEMENTED();
}

// Emit *dst = *lhs != *rhs. dst must have SizeClass BOOL.
void StreamBuilder::Neq(Mem, Mem, Mem) {
  UNIMPLEMENTED();
}

// Emit *dst = !*src. dst and src must have SizeClass BOOL.
void StreamBuilder::Not(Mem, Mem) {
  UNIMPLEMENTED();
}

Stream StreamBuilder::Build() const {
  UNIMPLEMENTED();
}

} // namespace ir
