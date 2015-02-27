#ifndef TYPES_SYMBOL_TABLE_H
#define TYPES_SYMBOL_TABLE_H

#include "ast/ast.h"
#include "base/fileset.h"
#include "types/type_info_map.h"
#include <map>

namespace types {

struct VariableInfo {
public:
  VariableInfo(
      ast::TypeId tid = ast::TypeId::kUnassigned,
      string name = "",
      base::PosRange pos = base::PosRange(-1, -1, -1),
      ast::LocalVarId vid = ast::kVarUnassigned)
    : tid(tid), name(name), pos(pos), vid(vid) {}

  ast::TypeId tid;
  string name;
  base::PosRange pos;
  ast::LocalVarId vid;
};

class SymbolTable {
public:
  SymbolTable(const base::FileSet* fs, const vector<VariableInfo>& params);

  static SymbolTable Empty() {
    return SymbolTable(&base::FileSet::Empty(), {});
  }


  void EnterScope();
  void LeaveScope();

  ast::LocalVarId DeclareLocalStart(ast::TypeId tid, const string& name, base::PosRange nameRange, base::ErrorList* errors);
  void DeclareLocalEnd(ast::LocalVarId vid);
  pair<ast::TypeId, ast::LocalVarId> ResolveLocal(const string& name, base::PosRange nameRange, base::ErrorList* errors) const;

private:
  base::Error* MakeUndefinedReferenceError(string varName, base::PosRange nameRange) const;
  base::Error* MakeDuplicateVarDeclError(string varName, base::PosRange varPos, base::PosRange originalVarPos) const;
  base::Error* MakeVariableInitializerSelfReferenceError(base::PosRange pos) const;

  const base::FileSet* fs_;
  std::map<string, VariableInfo> params_;
  std::map<string, VariableInfo> cur_symbols_; // Doesn't include params.
  u32 cur_scope_len_;
  vector<string> scopes_;
  vector<u32> scope_lengths_;
  ast::LocalVarId var_id_counter_;
  ast::LocalVarId currently_declaring_;
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

struct VarDeclGuard {
  VarDeclGuard(SymbolTable* symbolTable, ast::TypeId tid, const string& name, base::PosRange nameRange, base::ErrorList* errors) : symbol_table_(symbolTable) {
    vid_ = symbolTable->DeclareLocalStart(tid, name, nameRange, errors);
  }

  ~VarDeclGuard() {
    symbol_table_->DeclareLocalEnd(vid_);
  }

  ast::LocalVarId GetVarId() {
    return vid_;
  }

  SymbolTable* symbol_table_;
  ast::LocalVarId vid_;
};

} // namespace types

#endif
