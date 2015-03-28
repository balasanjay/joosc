#include "ir/size.h"

using ast::TypeId;

namespace ir {

SizeClass SizeClassFrom(TypeId tid) {
#define C(t, s) \
  if (tid == TypeId::k##t) { \
    return SizeClass::s; \
  }

  C(Bool, BOOL);
  C(Byte, BYTE);
  C(Short, SHORT);
  C(Char, CHAR);
  C(Int, INT);

#undef C

  return SizeClass::PTR;
}

u64 ByteSizeFrom(SizeClass size, u8 ptr_size) {
  switch (size) {
    case SizeClass::BOOL: // Fall through.
    case SizeClass::BYTE:
      return 1;
    case SizeClass::SHORT: // Fall through.
    case SizeClass::CHAR:
      return 2;
    case SizeClass::INT:
      return 4;
    case SizeClass::PTR:
      return (u64)ptr_size;
    default:
      UNREACHABLE();
  }
}

} // namespace ir
