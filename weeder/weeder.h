#ifndef WEEDER_WEEDER_H
#define WEEDER_WEEDER_H

#include "base/errorlist.h"
#include "base/fileset.h"
#include "ast/ast.h"

namespace weeder {

void WeedProgram(const base::FileSet* fs, const ast::Program* prog,
                 base::ErrorList* out);

}  // namespace weeder

#endif
