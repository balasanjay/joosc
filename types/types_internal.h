#ifndef TYPES_TYPES_INTERNAL_H
#define TYPES_TYPES_INTERNAL_H

#include "ast/ast_fwd.h"
#include "ast/ids.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "types/typeset.h"

namespace types {

base::Error* MakeUnknownTypenameError(const base::FileSet* fs, base::PosRange pos);
base::Error* MakeDuplicateDefinitionError(const base::FileSet* fs, const vector<base::PosRange> dupes, const string& main_message, const string& name);
sptr<const ast::Type> ResolveType(sptr<const ast::Type> type, TypeSet typeset, base::PosRange* pos_out);

} // namespace types

#endif
