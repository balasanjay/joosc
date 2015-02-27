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
    return std::tie(name, param_types) < std::tie(other.name, other.param_types);
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
  bool is_constructor;

  bool operator<(const MethodInfo& other) const {
    return std::tie(class_type, signature) < std::tie(other.class_type, other.signature);
  }
};

struct MethodTableParam {
  MethodInfo minfo;
  MethodId mid;
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

  MethodTable(const vector<MethodTableParam>& entries) {
    for (const auto& entry : entries) {
      InsertMethod(entry.mid, entry.minfo);
    }
  }

  static MethodTable kEmptyMethodTable;

  MethodSignatureMap method_signatures_;
  MethodInfoMap method_info_;
};

struct TypeInfo {
  ast::ModifierList mods;
  ast::TypeKind kind;
  ast::TypeId type;
  string name;
  base::PosRange pos;
  TypeIdList extends;
  TypeIdList implements;
  MethodTable methods;
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

  pair<const TypeInfo&, bool> LookupTypeInfo(ast::TypeId tid) {
    auto info = type_info_.find(tid);
    assert(info != type_info_.end());
    // TODO: Bool?
    return make_pair(info->second, true);
  }

private:
  friend class TypeInfoMapBuilder;

  void InsertTypeInfo(ast::TypeId tid, const TypeInfo& typeinfo) {
    type_info_.insert({tid, typeinfo});
  }

  TypeInfoMap(const vector<TypeInfo>& entries) {
    for (const auto& entry : entries) {
      InsertTypeInfo(entry.type, entry);
    }
  }

  static TypeInfoMap kEmptyTypeInfoMap;

  std::map<ast::TypeId, TypeInfo> type_info_;
};

class TypeInfoMapBuilder {
public:
  void PutType(ast::TypeId tid, const ast::TypeDecl& type, const vector<ast::TypeId>& extends, const vector<ast::TypeId>& implements) {
    assert(tid.ndims == 0);
    type_entries_.push_back(TypeEntry{type.Mods(), type.Kind(), tid, type.Name(), type.NameToken().pos, TypeIdList(extends), TypeIdList(implements)});
  }

  void PutMethod(ast::TypeId curtid, ast::TypeId rettid, const vector<ast::TypeId>& paramtids, const ast::MemberDecl& meth, bool is_constructor) {
    method_entries_.push_back(MethodInfo{curtid, meth.Mods(), rettid, meth.NameToken().pos, MethodSignature{meth.Name(), TypeIdList(paramtids)}, is_constructor});
  }

  TypeInfoMap Build(const base::FileSet* fs, base::ErrorList* out);

  base::Error* MakeConstructorNameError(const base::FileSet* fs, base::PosRange pos) const;

private:
  struct TypeEntry {
    ast::ModifierList mods;
    ast::TypeKind kind;
    ast::TypeId type;
    string name;
    base::PosRange pos;
    TypeIdList extends;
    TypeIdList implements;

    bool operator<(const TypeEntry& other) const {
      return type < other.type;
    }
  };

  vector<TypeEntry> type_entries_;
  vector<MethodInfo> method_entries_;
};

} // namespace types

#endif
