#ifndef TYPES_SYMBOL_TABLE_H
#define TYPES_SYMBOL_TABLE_H

#include "ast/ast.h"
#include "base/fileset.h"
#include "types/type_info_map.h"
#include <map>

namespace types {

using LocalVarId = u64;
const LocalVarId kVarUnassigned = 0;
const LocalVarId kVarFirst = 100;

struct VariableInfo {
  LocalVarId vid;
  ast::TypeId tid;
  string name;
  base::PosRange posRange;
};

class SymbolTable {
public:
  SymbolTable(const base::FileSet* fs, const TypeIdList& paramTids, const vector<string>& paramNames, const vector<base::PosRange>& ranges);

  void EnterScope();
  void LeaveScope();

  pair<ast::TypeId, LocalVarId> DeclareLocal(const ast::Type& type, const string& name, base::PosRange nameRange, base::ErrorList* errors);
  pair<ast::TypeId, LocalVarId> ResolveLocal(const string& name, base::PosRange nameRange, base::ErrorList* errors) const;

private:
  base::Error* MakeUndefinedReferenceError(string varName, base::PosRange nameRange) const;
  base::Error* MakeDuplicateVarDeclError(string varName, base::PosRange varPos, base::PosRange originalVarPos) const;

  const base::FileSet* fs_;
  std::map<string, VariableInfo> params_;
  std::map<string, VariableInfo> cur_symbols_; // Doesn't include params.
  vector<string> cur_scope_;
  vector<vector<string>> scopes_;
  LocalVarId var_id_counter_;
};

struct ScopeGuard {
public:
  ScopeGuard(SymbolTable* table) : table_(table) {
    table_->EnterScope();
  }
  ~ScopeGuard() {
    table_->LeaveScope();
  }
private:
  SymbolTable* table_;
};


} // namespace types

#endif
