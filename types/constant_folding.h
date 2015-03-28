#ifndef CONSTANT_FOLDING_H
#define CONSTANT_FOLDING_H

#include "ast/ids.h"
#include "ast/ast_fwd.h"

namespace types {

using StringId = u64;
using ConstStringMap = map<string, StringId>;

sptr<const ast::Program> ConstantFold(sptr<const ast::Program> prog, ast::TypeId string_type, ConstStringMap* out_strings);

} // namespace types

#endif
