#include "backend/i386/writer.h"

#include <iostream>

#include "base/printf.h"
#include "ir/mem.h"
#include "ir/stream.h"

using std::ostream;

using base::Fprintf;
using base::Sprintf;
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

struct FuncWriter final {
  FuncWriter(ostream* outarg) : out(outarg) {}

  template<typename... Args>
  void Col0(const string& fmt, Args... args) {
    Fprintf(out, fmt, args...);
    *out << '\n';
  }

  template<typename... Args>
  void Col1(const string& fmt, Args... args) {
    *out << "    ";
    Col0(fmt, args...);
  }

  void WritePrologue(const Stream& stream) {
    Col0("; Starting method.");

    if (stream.is_entry_point) {
      Col0("global _entry");
      Col0("_entry:");
    }

    string label = Sprintf("_t%v_m%v", stream.tid, stream.mid);

    Col0("global %v", label);
    Col0("%v:\n", label);

    Col1("; Function prologue.");
    Col1("push ebp");
    Col1("mov ebp, esp\n");
  }

  void WriteEpilogue() {
    // TODO: this is assuming that it was an int.
    Col0(".epilogue:");
    Col1("pop ebp");
    Col1("ret");
  }

  void AllocMem(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(3);

    MemId memid = begin[0];
    SizeClass size = (SizeClass)begin[1];
    // bool is_immutable = begin[2] == 1;

    // TODO: We always allocate 4 bytes. Fixes here will also affect dealloc.

    i64 offset = cur_offset;
    cur_offset += 4;

    Col1("; [ebp-%v] refers to t%v.", offset, memid);

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

    Col1("; t%v deallocated, used to be at [ebp-%v].", memid, entry.offset);
  }

  void Label(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(1);

    LabelId lid = begin[0];

    Col0(".L%v:", lid);
  }

  void Const(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(3);

    MemId memid = begin[0];
    SizeClass size = (SizeClass)begin[1];
    u64 value = begin[2];

    const StackEntry& entry = stack_map.at(memid);
    CHECK(entry.size == size);

    string mov_size = Sized(size, "byte", "word", "dword");

    Col1("; t%v = %v.", memid, value);
    Col1("mov %v [ebp-%v], %v", mov_size, entry.offset, value);
  }

  void Mov(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    MemId dst = begin[0];
    MemId src = begin[1];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& src_e = stack_map.at(src);
    CHECK(dst_e.size == src_e.size);

    string reg_size = Sized(dst_e.size, "al", "ax", "eax");

    Col1("; t%v = t%v.", dst_e.id, src_e.id);
    Col1("mov %v, [ebp-%v]", reg_size, src_e.offset);
    Col1("mov [ebp-%v], %v", dst_e.offset, reg_size);
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

    Col1("; t%v = t%v + t%v.", dst_e.id, lhs_e.id, rhs_e.id);
    Col1("mov eax, [ebp-%v]", lhs_e.offset);
    Col1("add eax, [ebp-%v]", rhs_e.offset);
    Col1("mov [ebp-%v], eax", dst_e.offset);
  }

  void Jmp(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(1);

    LabelId lid = begin[0];

    Col1("jmp .L%v", lid);
  }

  void JmpIf(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    LabelId lid = begin[0];
    MemId cond = begin[1];

    const StackEntry& cond_e = stack_map.at(cond);

    CHECK(cond_e.size == SizeClass::BOOL);

    Col1("; Jumping if t%v.", cond);
    Col1("mov al, [ebp-%v]", cond_e.offset);
    Col1("test al, al");
    Col1("jnz .L%v", lid);
  }

  void RelImpl(ArgIter begin, ArgIter end, const string& relation, const string& instruction) {
    EXPECT_NARGS(3);

    MemId dst = begin[0];
    MemId lhs = begin[1];
    MemId rhs = begin[2];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& lhs_e = stack_map.at(lhs);
    const StackEntry& rhs_e = stack_map.at(rhs);

    CHECK(dst_e.size == SizeClass::BOOL);
    CHECK(lhs_e.size == SizeClass::INT);
    CHECK(rhs_e.size == SizeClass::INT);

    Col1("; t%v = (t%v %v t%v).", dst_e.id, lhs_e.id, relation, rhs_e.id);
    Col1("mov eax, [ebp-%v]", lhs_e.offset);
    Col1("cmp eax, [ebp-%v]", rhs_e.offset);
    Col1("%v [ebp-%v]", instruction, dst_e.offset);
  }

  void Lt(ArgIter begin, ArgIter end) {
    RelImpl(begin, end, "<", "setl");
  }

  void Leq(ArgIter begin, ArgIter end) {
    RelImpl(begin, end, "<=", "setle");
  }

  void Eq(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(3);

    MemId dst = begin[0];
    MemId lhs = begin[1];
    MemId rhs = begin[2];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& lhs_e = stack_map.at(lhs);
    const StackEntry& rhs_e = stack_map.at(rhs);

    CHECK(dst_e.size == SizeClass::BOOL);
    CHECK(lhs_e.size == rhs_e.size);
    CHECK(lhs_e.size == SizeClass::BOOL ||
          lhs_e.size == SizeClass::INT ||
          lhs_e.size == SizeClass::PTR);

    string reg_size = Sized(lhs_e.size, "al", "", "eax");

    Col1("; t%v = (t%v == t%v).", dst_e.id, lhs_e.id, rhs_e.id);
    Col1("mov %v, [ebp-%v]", reg_size, lhs_e.offset);
    Col1("cmp %v, [ebp-%v]", reg_size, rhs_e.offset);
    Col1("sete [ebp-%v]", dst_e.offset);
  }

  void Not(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    MemId dst = begin[0];
    MemId src = begin[1];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& src_e = stack_map.at(src);

    CHECK(dst_e.size == SizeClass::BOOL);
    CHECK(src_e.size == SizeClass::BOOL);

    Col1("; t%v = !t%v", dst_e.id, src_e.id);
    Col1("mov al, [ebp-%v]", src_e.offset);
    Col1("xor al, 1");
    Col1("mov [ebp-%v], al", dst_e.offset);
  }

  void Ret(ArgIter begin, ArgIter end) {
    CHECK((end-begin) <= 1);

    if ((end - begin) == 1) {
      MemId ret = begin[0];
      const StackEntry& ret_e = stack_map.at(ret);

      string reg_size = Sized(ret_e.size, "al", "ax", "eax");

      Col1("; Return t%v.", ret_e.id);
      Col1("mov %v, [ebp-%v]", reg_size, ret_e.offset);
    } else {
      Col1("; Return.");
    }

    Col1("jmp .epilogue");
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
      case OpType::LT:
        writer.Lt(begin, end);
        break;
      case OpType::LEQ:
        writer.Leq(begin, end);
        break;
      case OpType::EQ:
        writer.Eq(begin, end);
        break;
      case OpType::NOT:
        writer.Not(begin, end);
        break;
      case OpType::RET:
        writer.Ret(begin, end);
        break;

      UNIMPLEMENTED_OP(MOV_ADDR);
      UNIMPLEMENTED_OP(SIGN_EXTEND);
      UNIMPLEMENTED_OP(ZERO_EXTEND);
      UNIMPLEMENTED_OP(TRUNCATE);

      default:
        CHECK(false); // Should be unreachable.
    }
  }

  writer.WriteEpilogue();

}


} // namespace i386
} // namespace backend
