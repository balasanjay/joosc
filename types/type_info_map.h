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
const MethodId kErrorMethodId = 0;
const MethodId kFirstMethodId = 1;

enum CallContext {
  INSTANCE,
  CONSTRUCTOR,
  STATIC,
};

struct MethodSignature {
  bool is_constructor;
  string name;
  TypeIdList param_types;

  bool operator<(const MethodSignature& other) const {
    return std::tie(is_constructor, name, param_types) < std::tie(other.is_constructor, other.name, other.param_types);
  }

  bool operator==(const MethodSignature& other) const {
    return !(*this < other) && !(other < *this);
  }
};

struct MethodInfo {
  MethodId mid;
  ast::TypeId class_type;
  ast::ModifierList mods;
  ast::TypeId return_type;
  base::PosRange pos;
  MethodSignature signature;
};

class MethodTable {
public:
  // TODO
  MethodId ResolveCall(ast::TypeId callerType, CallContext ctx, const TypeIdList& params, base::ErrorList* out) const;

  // Given a valid MethodId, return all the associated info about it.
  const MethodInfo& LookupMethod(MethodId mid) const {
    if (mid == kErrorMethodId) {
      return kErrorMethodInfo;
    }

    auto info = method_info_.find(mid);
    assert(info != method_info_.end());
    return info->second;
  }

private:
  friend class TypeInfoMapBuilder;
  using MethodSignatureMap = std::map<MethodSignature, MethodInfo>;
  using MethodInfoMap = std::map<MethodId, MethodInfo>;

  MethodTable(const MethodSignatureMap& entries, const set<string>& bad_methods, bool has_bad_constructor) : method_signatures_(entries), has_bad_constructor_(has_bad_constructor), bad_methods_(bad_methods) {
    for (const auto& entry : entries) {
      method_info_.insert({entry.second.mid, entry.second});
    }
  }

  MethodTable() : all_blacklisted_(true) {}

  static MethodTable kEmptyMethodTable;
  static MethodTable kErrorMethodTable;
  static MethodInfo kErrorMethodInfo;

  MethodSignatureMap method_signatures_;
  MethodInfoMap method_info_;

  // All blacklisting information.
  // Every call is blacklisted.
  bool all_blacklisted_ = false;
  // Any constructor is blacklisted.
  bool has_bad_constructor_ = false;
  // Specific method names are blacklisted.
  set<string> bad_methods_;
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

  // Orders all types in topological order such that if there is a type A that
  // implements or extends another type B, then B has a lower top_sort_index
  // than A.
  u64 top_sort_index;
};

class TypeInfoMap {
public:
  static const TypeInfoMap& Empty() {
    return kEmptyTypeInfoMap;
  }

  // TODO: handle blacklisting.
  pair<const TypeInfo&, bool> LookupTypeInfo(ast::TypeId tid) {
    const auto info = type_info_.find(tid);
    assert(info != type_info_.cend());
    return make_pair(info->second, true);
  }

private:
  using Map = map<ast::TypeId, TypeInfo>;
  friend class TypeInfoMapBuilder;

  TypeInfoMap(const Map& typeinfo) : type_info_(typeinfo) {}

  static TypeInfoMap kEmptyTypeInfoMap;
  static TypeInfo kEmptyTypeInfo;

  Map type_info_;
};

class TypeInfoMapBuilder {
public:
  TypeInfoMapBuilder(const base::FileSet* fs) : fs_(fs) {}

  void PutType(ast::TypeId tid, const ast::TypeDecl& type, const vector<ast::TypeId>& extends, const vector<ast::TypeId>& implements) {
    assert(tid.ndims == 0);
    type_entries_.push_back(TypeInfo{type.Mods(), type.Kind(), tid, type.Name(), type.NameToken().pos, TypeIdList(extends), TypeIdList(implements), MethodTable::kEmptyMethodTable, tid.base});
  }

  void PutMethod(ast::TypeId curtid, ast::TypeId rettid, const vector<ast::TypeId>& paramtids, const ast::MemberDecl& meth, bool is_constructor) {
    method_entries_.push_back(MethodInfo{kErrorMethodId, curtid, meth.Mods(), rettid, meth.NameToken().pos, MethodSignature{is_constructor, meth.Name(), TypeIdList(paramtids)}});
  }

  TypeInfoMap Build(base::ErrorList* out);

private:
  using MInfoIter = vector<MethodInfo>::iterator;
  using MInfoCIter = vector<MethodInfo>::const_iterator;

  MethodTable MakeResolvedMethodTable(TypeInfo* tinfo, const MethodTable::MethodSignatureMap& good_methods, const set<string>& bad_methods, bool has_bad_constructor, const map<ast::TypeId, TypeInfo>& sofar, base::ErrorList* out);

  void BuildMethodTable(MInfoIter begin, MInfoIter end, TypeInfo* tinfo, MethodId* cur_mid, const map<ast::TypeId, TypeInfo>& sofar, base::ErrorList* out);

  void ValidateExtendsImplementsGraph(map<ast::TypeId, TypeInfo>* m, base::ErrorList* errors);
  void PruneInvalidGraphEdges(const map<ast::TypeId, TypeInfo>&, set<ast::TypeId>*, base::ErrorList*);
  vector<ast::TypeId> VerifyAcyclicGraph(const multimap<ast::TypeId, ast::TypeId>&, set<ast::TypeId>*, std::function<void(const vector<ast::TypeId>&)>);

  base::Error* MakeConstructorNameError(base::PosRange pos) const;
  base::Error* MakeExtendsCycleError(const vector<TypeInfo>& cycle) const;

  const base::FileSet* fs_;
  vector<TypeInfo> type_entries_;
  vector<MethodInfo> method_entries_;
};

} // namespace types

#endif
