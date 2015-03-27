#ifndef TYPES_TYPES_H
#define TYPES_TYPES_H

#include "ast/ast_fwd.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace types {

class TypeInfoMap;

sptr<const ast::Program> TypecheckProgram(sptr<const ast::Program> prog,
                                          TypeInfoMap* tinfo_out,
                                          base::ErrorList* err_out);

}  // namespace types

#endif
