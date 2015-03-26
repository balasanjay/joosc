#include "backend/i386/writer.h"

#include <array>
#include <iostream>

#include "base/printf.h"
#include "ir/mem.h"
#include "ir/stream.h"

using std::array;
using std::ostream;

using base::Fprintf;
using base::Sprintf;
using ir::LabelId;
using ir::MemId;
using ir::Op;
using ir::OpType;
using ir::SizeClass;
using ir::Stream;
using ir::kInvalidMemId;

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

    for (auto& reg : regs) {
      if (reg.mem == memid) {
        reg.mem = kInvalidMemId;
      }
    }
  }

  void Label(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(1);

    LabelId lid = begin[0];

    SpillAllRegs();

    Col0(".L%v:", lid);
  }

  void Const(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(3);

    MemId memid = begin[0];
    SizeClass size = (SizeClass)begin[1];
    u64 value = begin[2];

    const StackEntry& entry = stack_map.at(memid);
    CHECK(entry.size == size);

    Col1("; t%v = %v.", memid, value);

    Reg* dst_reg = GetDstReg(memid, size);

    Col1("mov %v, %v", dst_reg->OfSize(size), value);
  }

  void MovImpl(ArgIter begin, ArgIter end, bool addr) {
    EXPECT_NARGS(2);

    MemId dst = begin[0];
    MemId src = begin[1];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& src_e = stack_map.at(src);
    CHECK(dst_e.size == src_e.size);

    if (addr) {
      Col1("; t%v = &t%v.", dst_e.id, src_e.id);
      Reg* dst_reg = GetDstReg(dst, dst_e.size);
      Col1("lea %v, [ebp-%v]", dst_reg->OfSize(dst_e.size), src_e.offset);
      return;
    }

    Col1("; t%v = t%v.", dst_e.id, src_e.id);
    Reg* dst_reg = GetDstReg(dst, dst_e.size);
    Col1("mov %v, %v", dst_reg->OfSize(dst_e.size), MemFromStackOrReg(src));
  }

  void Mov(ArgIter begin, ArgIter end) {
    MovImpl(begin, end, false);
  }

  void MovAddr(ArgIter begin, ArgIter end) {
    MovImpl(begin, end, true);
  }

  void MovToAddr(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    MemId dst = begin[0];
    MemId src = begin[1];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& src_e = stack_map.at(src);
    CHECK(dst_e.size == src_e.size);

    string dst_reg = Sized(dst_e.size, "al", "ax", "eax");
    string src_reg = Sized(src_e.size, "bl", "bx", "ebx");

    // TODO: figure out registerization for this.

    Col1("; *t%v = t%v.", dst_e.id, src_e.id);
    Col1("mov %v, %v", src_reg, MemFromStackOrReg(src));
    Col1("mov %v, [ebp-%v]", dst_reg, dst_e.offset);
    Col1("mov [%v], %v", dst_reg, src_reg);
  }

  void Arithmetic(ArgIter begin, ArgIter end, bool add) {
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

    string op_str = add ? "+" : "-";
    string instr = add ? "add" : "sub";

    Col1("; t%v = t%v %v t%v.", dst_e.id, lhs_e.id, op_str, rhs_e.id);
    Reg* dst_reg = GetDstReg(dst_e.id, dst_e.size);
    Col1("mov %v, %v", dst_reg->b4, MemFromStackOrReg(lhs));
    Col1("%v %v, %v", instr, dst_reg->b4, MemFromStackOrReg(rhs));
  }

  void Add(ArgIter begin, ArgIter end) {
    Arithmetic(begin, end, true);
  }

  void Sub(ArgIter begin, ArgIter end) {
    Arithmetic(begin, end, false);
  }

  void Mul(ArgIter begin, ArgIter end) {
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

    Col1("; t%v = t%v * t%v.", dst_e.id, lhs_e.id, rhs_e.id);

    SpillReg(&regs[0]);
    SpillReg(&regs[3]);

    Col1("mov eax, %v", MemFromStackOrReg(lhs));
    Col1("mov edx, %v", MemFromStackOrReg(rhs));
    Col1("imul edx");

    regs[0].mem = dst;
    regs[0].memsize = dst_e.size;
  }

  void DivMod(ArgIter begin, ArgIter end, bool div) {
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

    string op_str = div ? "/" : "%";
    string res_reg = div ? "eax" : "edx";

    Col1("; t%v = t%v %v t%v.", dst_e.id, lhs_e.id, op_str, rhs_e.id);

    SpillReg(&regs[0]);
    SpillReg(&regs[1]);
    SpillReg(&regs[3]);

    Col1("mov eax, %v", MemFromStackOrReg(lhs));
    Col1("cdq"); // Sign-extend EAX through to EDX.
    Col1("mov ebx, %v", MemFromStackOrReg(rhs));
    Col1("idiv ebx");

    if (div) {
      regs[0].mem = dst;
      regs[0].memsize = dst_e.size;
    } else {
      regs[3].mem = dst;
      regs[3].memsize = dst_e.size;
    }
  }

  void Div(ArgIter begin, ArgIter end) {
    DivMod(begin, end, true);
  }

  void Mod(ArgIter begin, ArgIter end) {
    DivMod(begin, end, false);
  }

  void Jmp(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(1);

    LabelId lid = begin[0];

    SpillAllRegs();

    Col1("jmp .L%v", lid);
  }

  void JmpIf(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    LabelId lid = begin[0];
    MemId cond = begin[1];

    const StackEntry& cond_e = stack_map.at(cond);

    CHECK(cond_e.size == SizeClass::BOOL);

    Col1("; Jumping if t%v.", cond);
  
    SpillAllRegs();

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
    Reg* dst_reg = GetDstReg(dst, dst_e.size);
    Col1("mov %v, %v", dst_reg->b4, MemFromStackOrReg(lhs));
    Col1("cmp %v, %v", dst_reg->b4, MemFromStackOrReg(rhs));
    Col1("%v %v", instruction, dst_reg->b1);
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
    Reg* dst_reg = GetDstReg(dst, dst_e.size);
    Col1("mov %v, %v", dst_reg->OfSize(dst_e.size), MemFromStackOrReg(lhs));
    Col1("cmp %v, %v", dst_reg->OfSize(dst_e.size), MemFromStackOrReg(rhs));
    Col1("sete %v", dst_reg->b1);
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
    Reg* dst_reg = GetDstReg(dst, dst_e.size);
    Col1("mov %v, %v", dst_reg->b1, MemFromStackOrReg(src));
    Col1("xor %v, 1", dst_reg->b1);
  }

  void BoolOpImpl(ArgIter begin, ArgIter end, const string& op_str, const string& instr) {
    EXPECT_NARGS(3);

    MemId dst = begin[0];
    MemId lhs = begin[1];
    MemId rhs = begin[2];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& lhs_e = stack_map.at(lhs);
    const StackEntry& rhs_e = stack_map.at(rhs);

    CHECK(dst_e.size == SizeClass::BOOL);
    CHECK(lhs_e.size == SizeClass::BOOL);
    CHECK(rhs_e.size == SizeClass::BOOL);

    Col1("; t%v = t%v %v t%v.", dst_e.id, lhs_e.id, op_str, rhs_e.id);
    Reg* dst_reg = GetDstReg(dst, dst_e.size);
    Col1("mov %v, %v", dst_reg->b1, MemFromStackOrReg(lhs));
    Col1("%v %v, %v", instr, dst_reg->b1, MemFromStackOrReg(rhs));
  }

  void And(ArgIter begin, ArgIter end) {
    BoolOpImpl(begin, end, "&", "and");
  }

  void Or(ArgIter begin, ArgIter end) {
    BoolOpImpl(begin, end, "|", "or");
  }

  void Xor(ArgIter begin, ArgIter end) {
    BoolOpImpl(begin, end, "^", "xor");
  }

  void Ret(ArgIter begin, ArgIter end) {
    CHECK((end-begin) <= 1);

    if ((end - begin) == 1) {
      MemId ret = begin[0];
      const StackEntry& ret_e = stack_map.at(ret);

      string reg_size = Sized(ret_e.size, "al", "ax", "eax");

      Col1("; Return t%v.", ret_e.id);
      auto* reg = &regs[0];
      SpillReg(reg);
      Col1("mov %v, %v", reg->OfSize(ret_e.size), MemFromStackOrReg(ret));
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

  struct Reg {
    MemId mem;
    SizeClass memsize;

    string b1;
    string b2;
    string b4;

    string OfSize(SizeClass size) {
      return Sized(size, b1, b2, b4);
    }
  };

  void SpillReg(Reg* reg) {
    CHECK(reg != nullptr);
    if (reg->mem == kInvalidMemId) {
      return;
    }

    Col1("; Spilling t%v", reg->mem);
    Col1("mov [ebp-%v], %v", stack_map.at(reg->mem).offset, reg->OfSize(reg->memsize));
  }

  void SpillAllRegs() {
    for (auto& reg : regs) {
      SpillReg(&reg);
    }
  }

  string MemFromStackOrReg(MemId mem) {
    for (auto& reg : regs) {
      if (reg.mem == mem) {
        return reg.OfSize(reg.memsize);
      }
    }
    return Sprintf("[ebp-%v]", stack_map.at(mem).offset);
  }

  Reg* GetDstReg(MemId mem, SizeClass size) {
    for (auto& reg : regs) {
      if (reg.mem == kInvalidMemId) {
        reg.mem = mem;
        reg.memsize = size;
        return &reg;
      }
    }

    // TODO: use LRU spilling, rather than always spilling ebx.
    auto reg = &regs[1];
    SpillReg(reg);
    reg->mem = mem;
    reg->memsize = size;
    return reg;
  }

  const i64 frame_offset = 8;

  map<MemId, StackEntry> stack_map;
  i64 cur_offset = frame_offset;
  vector<StackEntry> stack;

  array<Reg, 4> regs = {{
    {kInvalidMemId, SizeClass::INT, "al", "ax", "eax"},
    {kInvalidMemId, SizeClass::INT, "bl", "bx", "ebx"},
    {kInvalidMemId, SizeClass::INT, "cl", "cx", "ecx"},
    {kInvalidMemId, SizeClass::INT, "dl", "dx", "edx"},
  }};

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
      case OpType::MOV_ADDR:
        writer.MovAddr(begin, end);
        break;
      case OpType::MOV_TO_ADDR:
        writer.MovToAddr(begin, end);
        break;
      case OpType::ADD:
        writer.Add(begin, end);
        break;
      case OpType::SUB:
        writer.Sub(begin, end);
        break;
      case OpType::MUL:
        writer.Mul(begin, end);
        break;
      case OpType::DIV:
        writer.Div(begin, end);
        break;
      case OpType::MOD:
        writer.Mod(begin, end);
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
      case OpType::AND:
        writer.And(begin, end);
        break;
      case OpType::OR:
        writer.Or(begin, end);
        break;
      case OpType::XOR:
        writer.Xor(begin, end);
        break;
      case OpType::RET:
        writer.Ret(begin, end);
        break;

      UNIMPLEMENTED_OP(SIGN_EXTEND);
      UNIMPLEMENTED_OP(ZERO_EXTEND);
      UNIMPLEMENTED_OP(TRUNCATE);

      default:
        UNREACHABLE();
    }
  }

  writer.WriteEpilogue();

}


} // namespace i386
} // namespace backend
