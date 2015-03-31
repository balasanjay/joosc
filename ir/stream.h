#ifndef IR_STREAM_H
#define IR_STREAM_H

#include "ast/ids.h"
#include "ir/size.h"

namespace ir {

struct RuntimeLinkIds {
  ast::TypeId::Base type_info_type;
  ast::MethodId type_info_constructor;
  ast::TypeId::Base type_info_instanceof;
  ast::FieldId type_info_num_types;
};

// Numbered labels local to a function.
using LabelId = u64;

enum class OpType {
  // (Mem, SizeClass, bool is_immutable).
  ALLOC_MEM,

  // (Mem).
  DEALLOC_MEM,

  // (Mem, TypeId::Base).
  ALLOC_HEAP,

  // (Mem, SizeClass elemsize, Mem len).
  ALLOC_ARRAY,

  // (LabelId).
  LABEL,

  // (Mem, SizeClass, Value).
  CONST,

  // (Mem, Mem).
  MOV,

  // (Mem, Mem).
  MOV_ADDR,

  // (Mem, Mem).
  MOV_TO_ADDR,

  // (Mem, Mem, FieldId).
  FIELD_DEREF,

  // (Mem, Mem, FieldId).
  FIELD_ADDR,

  // (Mem, Mem, Mem, SizeClass).
  ARRAY_DEREF,

  // (Mem, Mem, Mem, SizeClass).
  ARRAY_ADDR,

  // (Mem, Mem, Mem).
  ADD,

  // (Mem, Mem, Mem).
  SUB,

  // (Mem, Mem, Mem).
  MUL,

  // (Mem, Mem, Mem).
  DIV,

  // (Mem, Mem, Mem).
  MOD,

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

  // (Mem, Mem).
  NOT,

  // (Mem, Mem).
  NEG,

  // (Mem, Mem, Mem).
  AND,

  // (Mem, Mem, Mem).
  OR,

  // (Mem, Mem, Mem).
  XOR,

  // (Mem, Mem).
  SIGN_EXTEND,

  // (Mem, Mem).
  ZERO_EXTEND,

  // (Mem, Mem, SizeClass).
  TRUNCATE,

  // (Mem, TypeId::Base, MethodId, int nargs, Mem[]).
  STATIC_CALL,

  // (Mem, Mem, MethodId, int nargs, Mem[]).
  DYNAMIC_CALL,

  // (Mem, Mem).
  GET_TYPEINFO,

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

  vector<SizeClass> params;
};

struct Type {
  ast::TypeId::Base tid;
  vector<Stream> streams;
};

struct CompUnit {
  string filename;
  vector<Type> types;
};

struct Program {
  vector<CompUnit> units;
  RuntimeLinkIds rt_ids;
};

} // namespace ir

#endif
