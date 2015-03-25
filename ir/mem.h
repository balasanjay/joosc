#ifndef IR_MEM_H
#define IR_MEM_H

#include "ir/stream.h"

namespace ir {

// Forward declared to avoid circular dep.
struct MemImpl;

using MemId = u64;

class Mem {
 public:
  MemId Id() const;
  SizeClass Size() const;

 private:
  sptr<MemImpl> impl_;
};

} // namespace ir

#endif
