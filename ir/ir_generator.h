#ifndef IR_IR_GENERATOR_H
#define IR_IR_GENERATOR_H

#include "ast/ast_fwd.h"
#include "ir/stream.h"
#include "types/type_info_map.h"
#include "types/types.h"

namespace ir {

Program GenerateIR(sptr<const ast::Program> program, const types::TypeSet& typeset, const types::TypeInfoMap& tinfo_map, const types::ConstStringMap& string_map);

} // namespace ir

#endif
