#ifndef TYPES_TYPES_H
#define TYPES_TYPES_H

#include "ast/ast_fwd.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "base/joos_types.h"

namespace types {

class TypeInfoMap;
class TypeSet;

using StringId = u64;
using ConstStringMap = map<jstring, StringId>;

sptr<const ast::Program> TypecheckProgram(sptr<const ast::Program> prog,
                                          TypeSet* typeset_out,
                                          TypeInfoMap* tinfo_out,
                                          ConstStringMap* string_map_out,
                                          base::ErrorList* err_out);

}  // namespace types

#endif
