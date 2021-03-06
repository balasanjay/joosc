#ifndef PARSER_PARSER_H
#define PARSER_PARSER_H

#include "ast/ast_fwd.h"
#include "base/errorlist.h"
#include "base/fileset.h"
#include "lexer/lexer.h"

namespace parser {

sptr<const ast::Program> Parse(const base::FileSet* fs,
                          const vector<vector<lexer::Token>>& tokens,
                          base::ErrorList* out);

string TokenString(const base::File* file, lexer::Token token);

} // namespace parser

#endif
