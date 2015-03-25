#ifndef IR_STREAM_H
#define IR_STREAM_H

#include "ast/ids.h"

namespace ir {

enum class SizeClass : u64 {
  BOOL,
  BYTE,
  SHORT,
  CHAR,
  INT,
  PTR,
};

// Numbered labels local to a function.
using LabelId = u64;

enum class OpType {
  // (Mem, SizeClass, bool is_immutable).
  ALLOC_MEM,

  // (Mem).
  DEALLOC_MEM,

  // (LabelId).
  LABEL,

  // (Mem, SizeClass, Value).
  CONST,

  // (Mem, Mem).
  MOV,

  // (Mem, Mem).
  MOV_ADDR,

  // (Mem, Mem, Mem).
  ADD,

  // (LabelId).
  JMP,

  // (LabelId, Mem).
  JMP_IF,

  // (Mem, Mem, Mem).
  LT,

  // (Mem, Mem, Mem).
  LEQ,

  // (Mem, Mem, Mem).
  EQ,

  // (Mem, Mem, Mem).
  NEQ,

  // (Mem, Mem).
  NOT,

  // (Mem, Mem).
  SIGN_EXTEND,

  // (Mem, Mem).
  ZERO_EXTEND,

  // (Mem, Mem, SizeClass).
  TRUNCATE,

  // ([Mem]).
  RET,
};

struct Op {
  OpType type;

  // Indices into args vector.
  size_t begin;
  size_t end;
};

struct Stream {
  bool is_entry_point;
  ast::TypeId::Base tid;
  ast::MethodId mid;
  vector<u64> args;
  vector<Op> ops;
};

} // namespace ir

#endif
