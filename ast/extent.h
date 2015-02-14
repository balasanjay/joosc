#ifndef AST_EXTENT_H
#define AST_EXTENT_H

#include "ast/ast_fwd.h"
#include "base/file.h"

namespace ast {

base::PosRange ExtentOf(sptr<const Expr> expr);
base::PosRange ExtentOf(sptr<const Stmt> stmt);

}  // namespace ast

#endif
