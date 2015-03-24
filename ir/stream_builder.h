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
    UNIMPLEMENTED();
  }

  Mem AllocLocal(SizeClass) {
    UNIMPLEMENTED();
  }

  // *dst = src.
  void Mov(Mem, Mem) {
    UNIMPLEMENTED();
  }

  // *dst = *lhs + *rhs.
  void Add(Mem, Mem, Mem) {
    UNIMPLEMENTED();
  }

 private:
  friend struct MemImpl;

  void DeallocMem(MemId) {
    UNIMPLEMENTED();
  }
};

} // namespace ir

#endif
