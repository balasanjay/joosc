#ifndef IR_MEM_IMPL_H
#define IR_MEM_IMPL_H

#include "ir/stream.h"

namespace ir {

class StreamBuilder;

struct MemImpl {
  ~MemImpl();

  u64 id;
  SizeClass size;
  StreamBuilder* builder;
};

} // namespace ir

#endif
