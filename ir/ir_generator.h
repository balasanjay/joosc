#ifndef DATAFLOW_VISITOR_H
#define DATAFLOW_VISITOR_H

#include "ast/ast_fwd.h"
#include "ir/stream.h"

namespace ir {

struct CompUnit {
  string filename;
  vector<Stream> streams;
};

struct Program {
  vector<CompUnit> units;
};

Program GenerateIR(sptr<const ast::Program> program);

} // namespace ir

#endif
