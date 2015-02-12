#ifndef TYPES_TYPECHECKER
#define TYPES_TYPECHECKER

#include "ast/ast.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace types {

uptr<ast::Program> TypecheckProgram(const ast::Program& prog,
                                    const base::FileSet* fs,
                                    base::ErrorList* out);

}  // namespace types

#endif
