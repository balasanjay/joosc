#include "opt/peep.h"

#include <algorithm>
#include <iostream>

#include "ir/mem.h"

using ir::MemId;
using ir::Op;
using ir::OpType;
using ir::Stream;

using ConstOpIter = vector<Op>::const_iterator;

namespace opt {

namespace {

void CopyOp(const Op& op, const Stream& src, Stream* out) {
  auto size_before = out->args.size();
  for (size_t i = op.begin; i < op.end; ++i) {
    out->args.push_back(src.args[i]);
  }
  out->ops.push_back({op.type, size_before, out->args.size()});
}

// Attempts to conservatively compute all mems written to by op, and writes
// them to out. Returns true iff this is a conservative estimate of all written
// mems. Otherwise, clients must assume that op may write to any memory.
bool GetWrites(const Op& op, const Stream& src, vector<MemId>* out) {
  auto push_arg = [&](size_t i) {
    CHECK(i < (op.end - op.begin));
    out->push_back(src.args[op.begin + i]);
  };

  switch (op.type) {
    case OpType::ALLOC_MEM:
    case OpType::DEALLOC_MEM:
      return true;

    case OpType::CONST:
    case OpType::MOV:
    case OpType::MOV_ADDR:
    case OpType::ADD:
    case OpType::LT:
    case OpType::LEQ:
    case OpType::EQ:
    case OpType::NOT:
    case OpType::SIGN_EXTEND:
    case OpType::ZERO_EXTEND:
    case OpType::TRUNCATE:
      push_arg(0);
      return true;

    default:
      return false;
  }
}

// Attempts to conservatively compute all mems read by op, and writes them to
// out. Returns true iff this is a conservative estimate of all read mems.
// Otherwise, clients must assume that op may read from any memory.
bool GetReads(const Op& op, const Stream& src, vector<MemId>* out) {
  auto push_arg = [&](size_t i) {
    CHECK(i < (op.end - op.begin));
    out->push_back(src.args[op.begin + i]);
  };

  switch (op.type) {
    case OpType::ALLOC_MEM:
    case OpType::DEALLOC_MEM:
    case OpType::CONST:
      return true;

    case OpType::MOV:
      push_arg(1);
      return true;

    case OpType::ADD:
    case OpType::LT:
    case OpType::LEQ:
    case OpType::EQ:
      push_arg(1);
      push_arg(2);
      return true;

    case OpType::NOT:
    case OpType::SIGN_EXTEND:
    case OpType::ZERO_EXTEND:
    case OpType::TRUNCATE:
      push_arg(1);
      return true;

    default:
      return false;
  }
}


void PeepholeBasicBlock(ConstOpIter begin, ConstOpIter end, const Stream& src, Stream* out) {
  // All immutable variables are written exactly once. If they are also read
  // exacly once via a MOV, then we can forward the original write directly to
  // the destination, and elide the temporary.
  set<MemId> candidate;
  map<MemId, MemId> provisional;
  map<MemId, MemId> confirmed
  for (auto cur = begin; cur != end; ++cur) {
    const Op& op = *cur;

    if (op.type == OpType::ALLOC_MEM) {
      // Check if this mem is declared immutable.
      if (src.args[op.begin + 2] == 1) {
        candidate.insert(src.args[op.begin]);
      }
      continue;
    }

    vector<MemId> reads;
    vector<MemId> writes;
    bool ok = GetReads(op, src, &reads) && GetWrites(op, src, &writes);
    CHECK(writes.size() <= 1);

    // If we can't figure out what we read or wrote, then just give up on
    // everything so far.
    if (!ok) {
      du_pairs.clear();
      continue;
    }

    for (auto read : reads) {
      // First, check if this is the second read of a mem. If so, then it is no
      // longer eligible.
      {
        auto iter = du_pairs.find(read);
        if (iter != du_pairs.end()) {
          du_pairs.erase(iter);
          continue;
        }
      }

      // Next, make sure that this mem is even a candidate for this
      // optimization. If not, then don't do anything with it.
      if (candidate.count(read) == 0) {
        continue;
      }

      // Reads by non-MOV instructions disqualify this candidate.
      if (op.type != OpType::MOV) {
        candidate.erase(read);
        continue;
      }

      // TODO(sjy): does writes.size() == 0 even make sense.
      CHECK(writes.size() == 1);

      // Move this mem from candidate list to provisional list.
      candidate.erase(read);
      du_pairs.insert({read, writes.at(0)});
    }
  }

  for (const auto& cand : du_pairs) {
    std::cout << "Candidate " << cand.first << "->" << cand.second << std::endl;
  }

  for (auto cur = begin; cur != end; ++cur) {
    CopyOp(*cur, src, out);
  }
}

} // namespace

Stream PeepholeOptimize(const Stream& oldstream) {
  // Copy the old stream, and remove all ops and args so we can rewrite them.
  Stream stream = oldstream;
  stream.args.clear();
  stream.ops.clear();

  // Tracking data flow across control flow is difficult. So we search for
  // blocks which don't have any control-flow. Control-flow includes label
  // definitions (which might be the target of jumps), and jumps.

  auto is_ctrl_flow = [](const Op& op) {
    switch (op.type) {
      case OpType::LABEL: // Fallthrough.
      case OpType::JMP: // Fallthrough.
      case OpType::JMP_IF:
        return true;
      default:
        return false;
    }
  };

  auto begin = oldstream.ops.begin();
  auto end = oldstream.ops.end();

  while (begin != end) {
    auto block_begin = find_if_not(begin, end, is_ctrl_flow);
    auto block_end = find_if(block_begin, end, is_ctrl_flow);

    // Copy all control-flow ops we skipped to stream.
    for (auto cur = begin; cur != block_begin; ++cur) {
      CopyOp(*cur, oldstream, &stream);
    }

    PeepholeBasicBlock(block_begin, block_end, oldstream, &stream);
    begin = block_end;
  }


  return stream;
}

} // namespace opt
