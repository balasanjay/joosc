#include "ir/mem.h"

#include "ir/mem_impl.h"

namespace ir {

MemId Mem::Id() const {
  return impl_->id;
}

SizeClass Mem::Size() const {
  return impl_->size;
}

} // namespace ir
