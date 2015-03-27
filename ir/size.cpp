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

} // namespace ir
