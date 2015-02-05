#ifndef WEEDER_WEEDER_H
#define WEEDER_WEEDER_H

#include "base/errorlist.h"
#include "base/fileset.h"
#include "parser/ast.h"

namespace weeder {

void WeedProgram(const base::FileSet* fs, const parser::Program* prog,
                 base::ErrorList* out);

}  // namespace weeder

#endif
