#ifndef TYPES_TYPES_TEST
#define TYPES_TYPES_TEST

#include "ast/ast_fwd.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace types {

sptr<const ast::Program> ParseProgramWithStdlib(base::FileSet** fs, const vector<pair<string, string>>& file_contents, base::ErrorList* out);

}  // namespace types

#endif
