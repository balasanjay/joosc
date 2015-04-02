#ifndef IR_STREAM_H
#define IR_STREAM_H

#include "ast/ids.h"
#include "ir/size.h"

namespace ir {

struct RuntimeLinkIds {
  ast::TypeId object_tid;

  ast::TypeId string_tid;
  ast::MethodId string_concat;
  map<ast::TypeId::Base, ast::MethodId> string_valueof;

  ast::TypeId type_info_tid;
  ast::MethodId type_info_constructor;
  ast::MethodId type_info_instanceof;
  ast::FieldId type_info_num_types;

  ast::TypeId stringops_type;
  ast::MethodId stringops_str;

  ast::TypeId stackframe_type;
  ast::MethodId stackframe_print;
  ast::MethodId stackframe_print_ex;
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

  // (Mem, StringId).
  CONST_STR,

  // (Mem, Mem).
  MOV,

  // (Mem, Mem).
  MOV_ADDR,

  // (Mem, Mem).
  MOV_TO_ADDR,

  // (Mem, Mem, TypeId::Base, FieldId, int file_offset).
  FIELD_DEREF,

  // (Mem, Mem, TypeId::Base, FieldId, int file_offset).
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

  // (Mem, Mem, Mem, int file_offset).
  DIV,

  // (Mem, Mem, Mem, int file_offset).
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
  EXTEND,

  // (Mem, Mem).
  TRUNCATE,

  // (Mem, Mem, TypeId::Base, TypeId::Ndims, TypeId::Base, TypeId::Ndims).
  INSTANCE_OF,

  // (Mem).
  CAST_EXCEPTION_IF_FALSE,

  // (Mem, Mem).
  CHECK_ARRAY_STORE,

  // (Mem, TypeId::Base, MethodId, int file_offset, int nargs, Mem[]).
  STATIC_CALL,

  // (Mem, Mem, MethodId, int file_offset, int nargs, Mem[]).
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
  int fileid;
};

struct Program {
  vector<CompUnit> units;
  RuntimeLinkIds rt_ids;
};

} // namespace ir

#endif
