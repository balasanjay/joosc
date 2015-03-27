#ifndef IR_SIZE_H
#define IR_SIZE_H

#include "ast/ids.h"

namespace ir {

enum class SizeClass : u8 {
  BOOL,
  BYTE,
  SHORT,
  CHAR,
  INT,
  PTR,
};

SizeClass SizeClassFrom(ast::TypeId tid);

} // namespace ir

#endif
