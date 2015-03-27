#include "backend/i386/writer.h"

#include <iostream>

#include "base/printf.h"
#include "ir/mem.h"
#include "ir/stream.h"

using std::ostream;

using ast::MethodId;
using ast::TypeId;
using base::Fprintf;
using base::Sprintf;
using ir::CompUnit;
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

// Convert our internal stack offset to an "[ebp-x]"-style string.
string StackOffset(i64 offset) {
  if (offset > 0) {
    return Sprintf("[ebp-%v]", offset);
  }
  return Sprintf("[ebp+%v]", (-offset) + 8);
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
      Col0("_entry:");
    }

    string label = Sprintf("_t%v_m%v", stream.tid, stream.mid);

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

  void SetupParams(const Stream& stream) {
    // TODO: figure out the correct initial offset.
    i64 param_offset = 0;
    for (size_t i = 0; i < stream.params.size(); ++i) {
      i64 cur_offset = param_offset;
      param_offset -= 4;

      StackEntry entry = {stream.params.at(i), cur_offset, i + 1};

      auto iter_pair = stack_map.insert({entry.id, entry});
      CHECK(iter_pair.second);
    }
  }

  void AllocMem(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(3);

    MemId memid = begin[0];
    SizeClass size = (SizeClass)begin[1];
    // bool is_immutable = begin[2] == 1;

    // TODO: We always allocate 4 bytes. Fixes here will also affect dealloc.

    i64 offset = cur_offset;
    cur_offset += 4;

    Col1("; %v refers to t%v.", StackOffset(offset), memid);

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

    Col1("; t%v deallocated, used to be at %v.", memid, StackOffset(entry.offset));
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
    Col1("mov %v %v, %v", mov_size, StackOffset(entry.offset), value);
  }

  void MovImpl(ArgIter begin, ArgIter end, bool addr) {
    EXPECT_NARGS(2);

    MemId dst = begin[0];
    MemId src = begin[1];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& src_e = stack_map.at(src);
    CHECK(dst_e.size == src_e.size);

    string reg_size = addr ? "eax" : Sized(dst_e.size, "al", "ax", "eax");
    string src_prefix = addr ? "&" : "";
    string instr = addr ? "lea" : "mov";

    Col1("; t%v = %vt%v.", dst_e.id, src_prefix, src_e.id);
    Col1("%v %v, %v", instr, reg_size, StackOffset(src_e.offset));
    Col1("mov %v, %v", StackOffset(dst_e.offset), reg_size);
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

    string src_reg = Sized(src_e.size, "bl", "bx", "ebx");

    Col1("; *t%v = t%v.", dst_e.id, src_e.id);
    Col1("mov %v, %v", src_reg, StackOffset(src_e.offset));
    Col1("mov eax, %v", StackOffset(dst_e.offset));
    Col1("mov [eax], %v", src_reg);
  }

  void AddSub(ArgIter begin, ArgIter end, bool add) {
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
    Col1("mov eax, %v", StackOffset(lhs_e.offset));
    Col1("%v eax, %v", instr, StackOffset(rhs_e.offset));
    Col1("mov %v, eax", StackOffset(dst_e.offset));
  }

  void Add(ArgIter begin, ArgIter end) {
    AddSub(begin, end, true);
  }

  void Sub(ArgIter begin, ArgIter end) {
    AddSub(begin, end, false);
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
    Col1("mov eax, %v", StackOffset(lhs_e.offset));
    Col1("mov ebx, %v", StackOffset(rhs_e.offset));
    Col1("imul ebx");
    Col1("mov %v, eax", StackOffset(dst_e.offset));
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
    Col1("mov eax, %v", StackOffset(lhs_e.offset));
    Col1("cdq"); // Sign-extend EAX through to EDX.
    Col1("mov ebx, %v", StackOffset(rhs_e.offset));
    Col1("idiv ebx");
    Col1("mov %v, %v", StackOffset(dst_e.offset), res_reg);
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

    Col1("jmp .L%v", lid);
  }

  void JmpIf(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    LabelId lid = begin[0];
    MemId cond = begin[1];

    const StackEntry& cond_e = stack_map.at(cond);

    CHECK(cond_e.size == SizeClass::BOOL);

    Col1("; Jumping if t%v.", cond);
    Col1("mov al, %v", StackOffset(cond_e.offset));
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
    Col1("mov eax, %v", StackOffset(lhs_e.offset));
    Col1("cmp eax, %v", StackOffset(rhs_e.offset));
    Col1("%v %v", instruction, StackOffset(dst_e.offset));
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
    Col1("mov %v, %v", reg_size, StackOffset(lhs_e.offset));
    Col1("cmp %v, %v", reg_size, StackOffset(rhs_e.offset));
    Col1("sete %v", StackOffset(dst_e.offset));
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
    Col1("mov al, %v", StackOffset(src_e.offset));
    Col1("xor al, 1");
    Col1("mov %v, al", StackOffset(dst_e.offset));
  }

  void Neg(ArgIter begin, ArgIter end) {
    EXPECT_NARGS(2);

    MemId dst = begin[0];
    MemId src = begin[1];

    const StackEntry& dst_e = stack_map.at(dst);
    const StackEntry& src_e = stack_map.at(src);

    CHECK(dst_e.size == SizeClass::INT);
    CHECK(src_e.size == SizeClass::INT);

    Col1("; t%v = -t%v", dst_e.id, src_e.id);
    Col1("mov eax, [ebp-%v]", src_e.offset);
    Col1("neg eax");
    Col1("mov [ebp-%v], eax", dst_e.offset);
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
    Col1("mov al, %v", StackOffset(lhs_e.offset));
    Col1("%v al, %v", instr, StackOffset(rhs_e.offset));
    Col1("mov %v, al", StackOffset(dst_e.offset));
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

  void StaticCall(ArgIter begin, ArgIter end) {
    CHECK((end-begin) >= 4);

    MemId dst = begin[0];
    TypeId::Base tid = begin[1];
    MethodId mid = begin[2];
    u64 nargs = begin[3];

    CHECK(((u64)(end-begin) - 4) == nargs);

    i64 param_offset = cur_offset;

    // Push args onto stack in reverse order.
    for (ArgIter cur = end; cur != (begin + 4); --cur) {
      MemId arg = *(cur - 1);
      const StackEntry& arg_e = stack_map.at(arg);

      string reg = Sized(arg_e.size, "al", "ax", "eax");
      Col1("mov %v, %v", reg, StackOffset(arg_e.offset));
      Col1("mov %v, %v", StackOffset(param_offset), reg);

      param_offset += 4;
    }

    Col1("sub esp, %v", param_offset);
    Col1("call t%v_m%v", tid, mid);
    Col1("add esp, %v", param_offset);
    Col1("mov %v, eax", StackOffset(stack_map.at(dst).offset));
  }

  void Ret(ArgIter begin, ArgIter end) {
    CHECK((end-begin) <= 1);

    if ((end - begin) == 1) {
      MemId ret = begin[0];
      const StackEntry& ret_e = stack_map.at(ret);

      string reg_size = Sized(ret_e.size, "al", "ax", "eax");

      Col1("; Return t%v.", ret_e.id);
      Col1("mov %v, %v", reg_size, StackOffset(ret_e.offset));
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

  const i64 frame_offset = 0;

  map<MemId, StackEntry> stack_map;
  i64 cur_offset = frame_offset;
  vector<StackEntry> stack;

  // TODO: do more optimal stack management for non-int-sized things.

  ostream* out;
};

} // namespace

void Writer::WriteCompUnit(const CompUnit& comp_unit, ostream* out) const {
  static string kMethodNameFmt = "_t%v_m%v";

  set<string> externs;
  set<string> globals;
  for (const Stream& method_stream : comp_unit.streams) {
    if (method_stream.is_entry_point) {
      globals.insert("_entry");
    }

    globals.insert(Sprintf(kMethodNameFmt, method_stream.tid, method_stream.mid));

    for (const Op& op : method_stream.ops) {
      // TODO: also will need things like static fields here.
      if (op.type == OpType::STATIC_CALL) {
        TypeId::Base tid = method_stream.args[op.begin+1];
        MethodId mid = method_stream.args[op.begin+2];

        externs.insert(Sprintf(kMethodNameFmt, tid, mid));
      }
    }
  }

  Fprintf(out, "; Predeclaring all necessary symbols.\n");
  for (const auto& global : globals) {
    // We cannot extern a symbol we are declaring in this file, so we remove
    // any local method calls from the externs map.
    externs.erase(global);

    Fprintf(out, "global %v\n", global);
  }
  for (const auto& ext : externs) {
    Fprintf(out, "extern %v\n", ext);
  }

  for (const ir::Stream& method_stream : comp_unit.streams) {
    WriteFunc(method_stream, out);
  }
}

void Writer::WriteFunc(const Stream& stream, ostream* out) const {
  FuncWriter writer{out};

  writer.WritePrologue(stream);

  writer.SetupParams(stream);

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
      case OpType::NEG:
        writer.Neg(begin, end);
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
      case OpType::STATIC_CALL:
        writer.StaticCall(begin, end);
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
