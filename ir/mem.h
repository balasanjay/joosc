#ifndef IR_MEM_H
#define IR_MEM_H

#include "ir/stream.h"

namespace ir {

// Forward declared to avoid circular dep.
struct MemImpl;

using MemId = u64;

class Mem {
 public:
  Mem(const Mem&) = default;

  MemId Id() const;
  SizeClass Size() const;
  bool Immutable() const;

 private:
  friend class StreamBuilder;

  Mem(sptr<MemImpl> impl) : impl_(impl) {}

  sptr<MemImpl> impl_;
};

} // namespace ir

#endif
