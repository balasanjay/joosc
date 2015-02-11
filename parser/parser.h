#ifndef PARSER_PARSER_H
#define PARSER_PARSER_H

#include "ast/ast_fwd.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "lexer/lexer.h"

namespace parser {

uptr<ast::Program> Parse(const base::FileSet* fs,
                          const vector<vector<lexer::Token>>& tokens,
                          base::ErrorList* out);

} // namespace parser

#endif
