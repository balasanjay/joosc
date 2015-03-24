#ifndef IR_MEM_H
#define IR_MEM_H

namespace ir {

// Forward declared to avoid circular dep.
struct MemImpl;

using MemId = u64;

class Mem {
 public:
  MemId Id() const;

  // TODO: size class.

 private:
  sptr<MemImpl> impl_;
};

} // namespace ir

#endif
