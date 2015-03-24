#include "ir/mem_impl.h"

#include "ir/mem.h"
#include "ir/stream_builder.h"

namespace ir {

MemImpl::~MemImpl() {
  builder->DeallocMem(id);
}

} // namespace ir
