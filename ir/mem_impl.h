#ifndef IR_MEM_IMPL_H
#define IR_MEM_IMPL_H

namespace ir {

class StreamBuilder;

struct MemImpl {
  ~MemImpl();

  u64 id;
  StreamBuilder* builder;
};

} // namespace ir

#endif
