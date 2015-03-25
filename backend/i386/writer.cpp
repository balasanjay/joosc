#include "backend/i386/writer.h"

#include <iostream>

#include "ir/mem.h"
#include "ir/stream.h"

using std::ostream;

using ir::LabelId;
using ir::MemId;
using ir::Op;
using ir::OpType;
using ir::SizeClass;
using ir::Stream;

#define EXPECT_NARGS(n) CHECK((end - begin) == n)

#define UNIMPLEMENTED_OP(type) \
  case OpType::type: \
    *out << "; Unimplemented op " << #type << '\n'; \
    *out << "ud2\n"; \
    break; \

namespace backend {
namespace i386 {

namespace {

using ArgIter = vector<u64>::const_iterator;

struct FuncWriter final {
  FuncWriter(ostream* outarg) : out(outarg) {}

  string Sized(SizeClass size, const string& b1, const string& b2, const string& b4) {
    switch(size) {
      case SizeClass::BOOL: // Fall through.
      case SizeClass::BYTE:
        return b1;
      case SizeClass::SHORT: // Fall through.
      case SizeClass::CHAR:
        return b2;
      case SizeClass::INT: // Fall through.
      case SizeClass::PTR:
        return b4;
      default:
        UNREACHABLE();
    }
  }

  void WritePrologue(const Stream& stream) {
    if (stream.is_entry_point) {
      *out << "global _entry\n";
      *out << "_entry:\n";
    }

    stringstream ss;
    ss << "_t" << stream.tid << "_m" << stream.mid;
    string label = ss.str();

    *out << "global " << label << '\n';
    *out << label << ":\n\n";

    *out << "; function prologue\n";
    *out << "push ebp\n";
    *out << "mov ebp, esp\n\n";
  }

  void AllocMem(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(3);

    MemId memid = begin[0];
    SizeClass size = (SizeClass)begin[1];
    // bool is_immutable = begin[2] == 1;

    // TODO: We always allocate 4 bytes. Fixes here will also affect dealloc.

    i64 offset = cur_offset;
    cur_offset += 4;

    *out << "; [ebp-" << offset << "] refers to t" << memid << '\n';

    StackEntry entry = {size, offset, memid};

    auto iter_pair = stack_map.insert({memid, entry});
    CHECK(iter_pair.second);

    stack.push_back(entry);
  }

  void DeallocMem(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(1);

    MemId memid = begin[0];

    CHECK(stack.size() > 0);
    auto entry = stack.back();

    CHECK(entry.id == memid);
    stack.pop_back();

    stack_map.erase(memid);

    cur_offset -= 4;
    CHECK(cur_offset >= frame_offset);

    *out << "; t" << memid << " deallocated, used to be at [ebp-" << entry.offset << "]\n";
  }

  void Label(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(1);

    LabelId lid = begin[0];

    *out << ".L" << lid << ":\n";
  }

  void Const(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(3);

    MemId memid = begin[0];
    SizeClass size = (SizeClass)begin[1];
    u64 value = begin[2];

    const StackEntry& entry = stack_map.at(memid);
    CHECK(entry.size == size);

    string mov_size = Sized(size, "byte", "word", "dword");

    *out << "; t" << memid << " = " << value << '\n';
    *out << "mov " << mov_size << " [ebp-" << entry.offset << "], " << value << '\n';
  }

  void Mov(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    MemId dst = begin[0];
    MemId src = begin[1];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& src_e = stack_map.at(src);
    CHECK(dst_e.size == src_e.size);

    string reg_size = Sized(dst_e.size, "al", "ax", "eax");

    *out << "; t" << dst_e.id << " = t" << src_e.id << '\n';
    *out << "mov " << reg_size << ", [ebp-" << src_e.offset << "]\n";
    *out << "mov [ebp-" << dst_e.offset << "], " << reg_size << '\n';
  }

  void Add(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(3);

    MemId dst = begin[0];
    MemId lhs = begin[1];
    MemId rhs = begin[2];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& lhs_e = stack_map.at(lhs);
    const StackEntry& rhs_e = stack_map.at(rhs);

    CHECK(dst_e.size == SizeClass::INT);
    CHECK(lhs_e.size == SizeClass::INT);
    CHECK(rhs_e.size == SizeClass::INT);

    *out << "; t" << dst_e.id << " = t" << lhs_e.id << " + t" << rhs_e.id << '\n';
    *out << "mov eax, [ebp-" << lhs_e.offset << "]\n";
    *out << "add eax, [ebp-" << rhs_e.offset << "]\n";
    *out << "mov [ebp-" << dst_e.offset << "], eax\n";
  }

  void Jmp(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(1);

    LabelId lid = begin[0];

    *out << "jmp .L" << lid << '\n';
  }

  void JmpIf(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    LabelId lid = begin[0];
    MemId cond = begin[1];

    const StackEntry& cond_e = stack_map.at(cond);

    CHECK(cond_e.size == SizeClass::BOOL);

    *out << "; Jumping if t" << cond << ".\n";
    *out << "mov al, [ebp-" << cond_e.offset << "]\n";
    *out << "tst al, al\n";
    *out << "jnz .L" << lid << '\n';
  }

 private:
  struct StackEntry {
    SizeClass size;
    i64 offset;
    MemId id;
  };

  const i64 frame_offset = 8;

  map<MemId, StackEntry> stack_map;
  i64 cur_offset = frame_offset;
  vector<StackEntry> stack;

  // TODO: do more optimal stack management for non-int-sized things.

  ostream* out;
};

} // namespace

void Writer::WriteFunc(const Stream& stream, ostream* out) const {
  FuncWriter writer{out};

  writer.WritePrologue(stream);

  for (const Op& op : stream.ops) {
    ArgIter begin = stream.args.begin() + op.begin;
    ArgIter end = stream.args.begin() + op.end;

    switch (op.type) {
      case OpType::ALLOC_MEM:
        writer.AllocMem(begin, end);
        break;
      case OpType::DEALLOC_MEM:
        writer.DeallocMem(begin, end);
        break;
      case OpType::LABEL:
        writer.Label(begin, end);
        break;
      case OpType::CONST:
        writer.Const(begin, end);
        break;
      case OpType::MOV:
        writer.Mov(begin, end);
        break;
      case OpType::ADD:
        writer.Add(begin, end);
        break;
      case OpType::JMP:
        writer.Jmp(begin, end);
        break;
      case OpType::JMP_IF:
        writer.JmpIf(begin, end);
        break;

      UNIMPLEMENTED_OP(MOV_ADDR);
      UNIMPLEMENTED_OP(LT);
      UNIMPLEMENTED_OP(LEQ);
      UNIMPLEMENTED_OP(EQ);
      UNIMPLEMENTED_OP(NEQ);
      UNIMPLEMENTED_OP(NOT);
      UNIMPLEMENTED_OP(SIGN_EXTEND);
      UNIMPLEMENTED_OP(ZERO_EXTEND);
      UNIMPLEMENTED_OP(TRUNCATE);

      default:
        CHECK(false); // Should be unreachable.
    }
  }

  // TODO: this is assuming that it was an int.
  *out << "mov eax, [ebp-8]\n\n";
  *out << ".epilogue:\n";
  *out << "pop ebp\n";
  *out << "ret\n";
}


} // namespace i386
} // namespace backend
