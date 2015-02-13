#ifndef TYPES_TYPECHECKER
#define TYPES_TYPECHECKER

#include "ast/ast.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace types {

const sptr<ast::Program> TypecheckProgram(const sptr<ast::Program> prog,
                                          const base::FileSet* fs,
                                          base::ErrorList* out);

}  // namespace types

#endif
