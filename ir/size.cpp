#include "ir/size.h"

using ast::TypeId;

namespace ir {

SizeClass SizeClassFrom(TypeId tid) {
  if (tid == TypeId::kInt) {
    return SizeClass::INT;
  } else if (tid == TypeId::kBool) {
    return SizeClass::BOOL;
  }
  // TODO.
  return SizeClass::INT;
}

} // namespace ir
