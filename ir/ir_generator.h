#ifndef IR_IR_GENERATOR_H
#define IR_IR_GENERATOR_H

#include "ast/ast_fwd.h"
#include "ir/stream.h"

namespace ir {

Program GenerateIR(sptr<const ast::Program> program);

} // namespace ir

#endif
