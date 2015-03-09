#ifndef WEEDER_WEEDER_H
#define WEEDER_WEEDER_H

#include "ast/ast.h"
#include "base/errorlist.h"

namespace weeder {

sptr<const ast::Program> WeedProgram(const base::FileSet* fs, sptr<const ast::Program> prog,
                 base::ErrorList* out);

}  // namespace weeder

#endif
