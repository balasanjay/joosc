#ifndef IR_STREAM_BUILDER_H
#define IR_STREAM_BUILDER_H

#include "ir/mem.h"
#include "ir/stream.h"

namespace ir {

// Forward declared to avoid circular dep.
struct MemImpl;

class StreamBuilder {
 public:
  Mem AllocTemp(SizeClass) {
    CHECK(false);
  }

  Mem AllocLocal(SizeClass) {
    CHECK(false);
  }

  // *dst = src.
  void Mov(Mem, Mem) {
    CHECK(false);
  }

  // *dst = *lhs + *rhs.
  void Add(Mem, Mem, Mem) {
    CHECK(false);
  }

 private:
  friend struct MemImpl;

  void DeallocMem(MemId) {
    CHECK(false);
  }
};

} // namespace ir

#endif
