#include "types/symbol_table.h"

#include "ast/extent.h"
#include "base/error.h"
#include "types/types_internal.h"

namespace types {

using ast::LocalVarId;
using ast::Type;
using ast::TypeId;
using ast::kVarFirst;
using ast::kVarUnassigned;
using base::Error;
using base::ErrorList;
using base::PosRange;

SymbolTable::SymbolTable(const base::FileSet* fs, const vector<VariableInfo>& params, ErrorList* errors)
  : fs_(fs), cur_scope_len_(0), currently_declaring_(kVarUnassigned) {
  var_id_counter_ = kVarFirst;

  // Enter param scope.
  EnterScope();

  for (const VariableInfo& param : params) {
    VariableInfo var_info(
      param.tid,
      param.name,
      param.pos,
      var_id_counter_
    );
    auto inserted = cur_symbols_.insert({var_info.name, var_info});
    if (!inserted.second) {
      errors->Append(MakeDuplicateVarDeclError(param.name, inserted.first->second.pos, param.pos));
    }
    ++var_id_counter_;
  }
}

SymbolTable::~SymbolTable() {
  // Leave param scope.
  LeaveScope();
  CHECK(scopes_.empty());
}

LocalVarId SymbolTable::DeclareLocalStart(ast::TypeId tid, const string& name, PosRange name_pos, ErrorList* errors) {
  CHECK(currently_declaring_ == kVarUnassigned);

  // Check if already defined.
  auto previousDef = cur_symbols_.find(name);
  if (previousDef != cur_symbols_.end()) {
    VariableInfo varInfo = previousDef->second;
    errors->Append(MakeDuplicateVarDeclError(name, name_pos, varInfo.pos));
    return varInfo.vid;
  }

  // Add new variable to current scope.
  VariableInfo varInfo = VariableInfo(
      tid,
      name,
      name_pos,
      var_id_counter_);
  currently_declaring_ = var_id_counter_;
  ++var_id_counter_;
  cur_symbols_[name] = varInfo;
  scopes_.push_back(name);
  ++cur_scope_len_;
  return varInfo.vid;
}

void SymbolTable::DeclareLocalEnd() {
  currently_declaring_ = kVarUnassigned;
}

const VariableInfo* SymbolTable::LookupVar(string name) const {
  auto findVar = cur_symbols_.find(name);
  if (findVar != cur_symbols_.end()) {
    return &findVar->second;
  }
  return nullptr;
}

pair<TypeId, LocalVarId> SymbolTable::ResolveLocal(const string& name, PosRange name_pos, ErrorList* errors) const {
  const VariableInfo* var = LookupVar(name);
  if (var == nullptr) {
    errors->Append(MakeUndefinedReferenceError(name, name_pos));
    return make_pair(TypeId::kUnassigned, kVarUnassigned);
  }

  // Check if currently in this variable's initializer.
  if (currently_declaring_ == var->vid) {
    errors->Append(MakeVariableInitializerSelfReferenceError(name_pos));
    return make_pair(TypeId::kUnassigned, kVarUnassigned);
  }

  return make_pair(var->tid, var->vid);
}

void SymbolTable::EnterScope() {
  scope_lengths_.push_back(cur_scope_len_);
  cur_scope_len_ = 0;
}

void SymbolTable::LeaveScope() {
  CHECK(scopes_.size() >= cur_scope_len_
      && !scope_lengths_.empty());
  for (u32 i = 0; i < cur_scope_len_; ++i) {
    string name = scopes_.back();
    scopes_.pop_back();
    auto found = cur_symbols_.find(name);
    CHECK(found != cur_symbols_.end());
    cur_symbols_.erase(found);
  }
  cur_scope_len_ = scope_lengths_.back();
  scope_lengths_.pop_back();
}

Error* SymbolTable::MakeUndefinedReferenceError(string name, PosRange pos) const {
  stringstream ss;
  ss << "Undefined reference to '";
  ss << name;
  ss << '\'';
  return MakeSimplePosRangeError(fs_, pos, "UndefinedReferenceError", ss.str());
}

Error* SymbolTable::MakeDuplicateVarDeclError(string name, PosRange pos, PosRange old_pos) const {
  stringstream msgstream;
  msgstream << "Local variable '" << name << "' was declared multiple times.";
  return MakeDuplicateDefinitionError(fs_, {pos, old_pos}, msgstream.str(), name);
}

Error* SymbolTable::MakeVariableInitializerSelfReferenceError(PosRange pos) const {
  return MakeSimplePosRangeError(fs_, pos, "VariableInitializerSelfReferenceError", "A variable cannot be used in its own initializer.");
}

} // namespace types
