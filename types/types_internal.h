#ifndef TYPES_TYPES_INTERNAL_H
#define TYPES_TYPES_INTERNAL_H

#include "ast/ast_fwd.h"
#include "ast/ids.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "types/typeset.h"

namespace types {

extern const base::PosRange kFakePos;
extern const lexer::Token kPublic;
extern const lexer::Token kProtected;
extern const lexer::Token kFinal;
extern const lexer::Token kAbstract;

base::Error* MakeUnknownTypenameError(base::PosRange pos);
base::Error* MakeDuplicateDefinitionError(const vector<base::PosRange> dupes, const string& main_message, const string& name);
base::Error* MakeDuplicateInheritanceError(bool is_extends, base::PosRange pos, ast::TypeId base_tid, ast::TypeId inheriting_tid);

sptr<const ast::Type> ResolveType(sptr<const ast::Type>, const TypeSet&, base::ErrorList*);

ast::ModifierList MakeModifierList(bool is_protected, bool is_final, bool is_abstract);

} // namespace types

#endif
