#ifndef IR_MEM_H
#define IR_MEM_H

#include "ir/stream.h"

namespace ir {

// Forward declared to avoid circular dep.
struct MemImpl;

using MemId = u64;
const MemId kInvalidMemId = 0;
const MemId kFirstMemId = 1;

class Mem {
 public:
  Mem(const Mem&) = default;

  MemId Id() const;
  SizeClass Size() const;
  bool Immutable() const;
  bool IsValid() const {
    return Id() != kInvalidMemId;
  }

 private:
  friend class StreamBuilder;

  Mem(sptr<MemImpl> impl) : impl_(impl) {}

  sptr<MemImpl> impl_;
};

} // namespace ir

#endif
