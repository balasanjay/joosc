#ifndef TYPES_TYPE_INFO_MAP_H
#define TYPES_TYPE_INFO_MAP_H

#include <algorithm>
#include <map>

#include "ast/ast.h"
#include "ast/ids.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace types {

struct TypeIdList {
public:
  TypeIdList(const vector<ast::TypeId>& tids) : tids_(tids){}
  TypeIdList(const std::initializer_list<ast::TypeId>& tids) : tids_(tids){}

  int Size() const {
    return tids_.size();
  }

  ast::TypeId At(int i) const {
    return tids_.at(i);
  }

  bool operator<(const TypeIdList& other) const {
    return std::lexicographical_compare(tids_.begin(), tids_.end(), other.tids_.begin(), other.tids_.end());
  }

  bool operator==(const TypeIdList& other) const {
    return tids_ == other.tids_;
  }
private:
  vector<ast::TypeId> tids_;
};

using MethodId = u64;
using LocalVarId = u64;

enum CallContext {
  INSTANCE,
  CONSTRUCTOR,
  STATIC,
};

struct MethodSignature {
  string name;
  TypeIdList param_types;

  bool operator<(const MethodSignature& other) const {
    return name < other.name || (name == other.name && param_types < other.param_types);
  }

  bool operator==(const MethodSignature& other) const {
    return name == other.name && param_types == other.param_types;
  }
};

struct MethodInfo {
  ast::TypeId class_type;
  ast::ModifierList mods;
  ast::TypeId return_type;
  base::PosRange namepos;
  MethodSignature signature;

  bool operator<(const MethodInfo& other) const {
    return signature < other.signature;
  }
};

class MethodTable {
public:
  // TODO
  MethodId ResolveCall(ast::TypeId callerType, CallContext ctx, const TypeIdList& params, base::ErrorList* out) const;

  // Given a valid MethodId, return all the associated info about it.
  const MethodInfo& LookupMethod(MethodId mid) const {
    auto info = method_info_.find(mid);
    assert(info != method_info_.end());
    return info->second;
  }

private:
  friend class TypeInfoMapBuilder;
  using MethodSignatureMap = std::map<MethodSignature, MethodId>;
  using MethodInfoMap = std::map<MethodId, MethodInfo>;

  void InsertMethod(MethodId mid, const MethodInfo& minfo) {
    method_signatures_.insert({minfo.signature, mid});
    method_info_.insert({mid, minfo});
  }

  MethodTable() = default;

  static MethodTable kEmptyMethodTable;

  MethodSignatureMap method_signatures_;
  MethodInfoMap method_info_;
};

struct TypeInfo {
  ast::ModifierList mods;
  ast::TypeKind kind;
  ast::TypeId type;
  string name;
  base::PosRange namePos;
  TypeIdList extends;
  TypeIdList implements;
  MethodTable mtable;
};

class SymbolTable {
public:
  SymbolTable(const TypeIdList& params);

  void EnterScope();
  void LeaveScope();

  pair<ast::TypeId, LocalVarId> DeclareLocal(const ast::Type& type, const string& name, base::ErrorList* out);
  pair<ast::TypeId, LocalVarId> ResolveLocal(const string& name, base::ErrorList* out) const;
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

class TypeInfoMap {
public:
  static const TypeInfoMap& Empty() {
    return kEmptyTypeInfoMap;
  }

  pair<const TypeInfo&, bool> LookupTypeInfo(ast::TypeId tid);

private:
  friend class TypeInfoMapBuilder;

  TypeInfoMap() = default;

  static TypeInfoMap kEmptyTypeInfoMap;
};

class TypeInfoMapBuilder {
public:
  // TODO: decide on an API.
  void Put(ast::TypeKind /*kind*/, ast::TypeId tid, const string& /*qualifiedname*/, const ast::ModifierList& /*mods*/) {
    assert(tid.ndims == 0);
  }

  void PutMethod(ast::TypeId curtid, ast::TypeId rettid, const vector<ast::TypeId>& paramtids, const ast::MemberDecl& meth) {
    method_entries_.push_back(MethodInfo{curtid, meth.Mods(), rettid, meth.NameToken().pos, MethodSignature{meth.Name(), TypeIdList(paramtids)}});
  }

  TypeInfoMap Build(const base::FileSet* fs, base::ErrorList* out) {
    CheckMethodDuplicates(fs, out);
    // TODO: Populate TypeInfo with a MethodTable.
    // TODO: Create TypeInfos.
    return TypeInfoMap();
  }

private:
  void CheckMethodDuplicates(const base::FileSet* fs, base::ErrorList* out);

  vector<MethodInfo> method_entries_;
};

} // namespace types

#endif
