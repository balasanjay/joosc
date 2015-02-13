#ifndef TYPES_TYPECHECKER_H
#define TYPES_TYPECHECKER_H

#include "ast/ast.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace types {

sptr<const ast::Program> TypecheckProgram(sptr<const ast::Program> prog,
                                          const base::FileSet* fs,
                                          base::ErrorList* out);

}  // namespace types

#endif
