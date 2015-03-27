#ifndef TYPES_TYPE_INFO_MAP_H
#define TYPES_TYPE_INFO_MAP_H

#include <algorithm>
#include <map>

#include "ast/ast.h"
#include "ast/ids.h"
#include "base/errorlist.h"
#include "types/typeset.h"

namespace types {

using ast::FieldId;
using ast::MethodId;
using ast::kErrorFieldId;
using ast::kErrorMethodId;
using ast::kFirstFieldId;
using ast::kFirstMethodId;

ast::ModifierList MakeModifierList(bool is_protected, bool is_final, bool is_abstract);

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

TypeIdList Concat(const std::initializer_list<TypeIdList>& types);

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

class TypeInfoMap;

class MethodTable {
public:
  MethodId ResolveCall(const TypeInfoMap& type_info_map, ast::TypeId caller_type, CallContext ctx, ast::TypeId callee_type, const TypeIdList& params, const string& method_name, base::PosRange pos, base::ErrorList* out) const;

  // Given a valid MethodId, return all the associated info about it.
  const MethodInfo& LookupMethod(MethodId mid) const {
    if (mid == kErrorMethodId) {
      return kErrorMethodInfo;
    }

    auto info = method_info_.find(mid);
    CHECK(info != method_info_.end());
    return info->second;
  }

  // Given a method name and params, return all the associated info about it.
  const MethodInfo& LookupMethod(const MethodSignature& msig) const {
    auto info = method_signatures_.find(msig);
    if (info == method_signatures_.end()) {
      CHECK(bad_methods_.count(msig.name) == 1);
      return kErrorMethodInfo;
    }
    return info->second;
  }

private:
  friend class TypeInfoMap;
  friend class TypeInfoMapBuilder;

  using MethodSignatureMap = std::map<MethodSignature, MethodInfo>;
  using MethodInfoMap = std::map<MethodId, MethodInfo>;

  MethodTable(const MethodSignatureMap& entries, const set<string>& bad_methods, bool has_bad_constructor) : method_signatures_(entries), has_bad_constructor_(has_bad_constructor), bad_methods_(bad_methods) {
    for (const auto& entry : entries) {
      method_info_.insert({entry.second.mid, entry.second});
    }
  }

  MethodTable() : all_blacklisted_(true) {}

  bool IsBlacklisted(CallContext ctx, const string& name) const;

  base::Error* MakeUndefinedMethodError(const TypeInfoMap& tinfo_map, const MethodSignature& sig, base::PosRange pos) const;

  base::Error* MakeInstanceMethodOnStaticError(base::PosRange pos) const;
  base::Error* MakeStaticMethodOnInstanceError(base::PosRange pos) const;
  base::Error* MakePermissionError(base::PosRange call_pos, base::PosRange method_pos) const;
  base::Error* MakeNewAbstractClassError(base::PosRange pos) const;

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

struct FieldInfo {
  FieldId fid;
  ast::TypeId class_type;
  ast::ModifierList mods;
  ast::TypeId field_type;
  base::PosRange pos;
  string name;
};

class FieldTable {
public:
  FieldId ResolveAccess(const TypeInfoMap& type_info_map, ast::TypeId callerType, CallContext ctx, ast::TypeId callee_type, string field_name, base::PosRange pos, base::ErrorList* out) const;

  // Given a valid FieldId, return all the associated info about it.
  const FieldInfo& LookupField(FieldId fid) const {
    if (fid == kErrorFieldId) {
      return kErrorFieldInfo;
    }

    auto info = field_info_.find(fid);
    CHECK(info != field_info_.end());
    return info->second;
  }

  // Given a field name, return all the associated info about it (no access checks).
  const FieldInfo& LookupField(string field_name) const {
    auto info = field_names_.find(field_name);
    if (info == field_names_.end()) {
      CHECK(bad_fields_.count(field_name) == 1);
      return kErrorFieldInfo;
    }
    return info->second;
  }

  const map<FieldId, FieldInfo>& GetFieldMap() const {
    return field_info_;
  }

private:
  friend class TypeInfoMapBuilder;
  friend class TypeInfoMap;

  using FieldNameMap = std::map<string, FieldInfo>;
  using FieldInfoMap = std::map<FieldId, FieldInfo>;

  FieldTable(const FieldNameMap& entries, const set<string>& bad_fields) : field_names_(entries), bad_fields_(bad_fields) {
    for (const auto& entry : entries) {
      field_info_.insert({entry.second.fid, entry.second});
    }
  }

  FieldTable() : all_blacklisted_(true) {}

  base::Error* MakeUndefinedReferenceError(const string& name, base::PosRange name_pos) const;
  base::Error* MakeInstanceFieldOnStaticError(base::PosRange pos) const;
  base::Error* MakeStaticFieldOnInstanceError(base::PosRange pos) const;
  base::Error* MakePermissionError(base::PosRange access_pos, base::PosRange field_pos) const;

  static FieldTable kEmptyFieldTable;
  static FieldTable kErrorFieldTable;
  static FieldInfo kErrorFieldInfo;

  FieldNameMap field_names_;
  FieldInfoMap field_info_;

  // All blacklisting information.
  // Every field is blacklisted.
  bool all_blacklisted_ = false;
  // Specific field names are blacklisted.
  set<string> bad_fields_;
};

struct TypeInfo {
  ast::ModifierList mods;
  ast::TypeKind kind;
  ast::TypeId type;
  string name;
  string package;
  base::PosRange pos;
  TypeIdList extends;
  TypeIdList implements;
  MethodTable methods;
  FieldTable fields;

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

  const TypeInfo& LookupTypeInfo(ast::TypeId tid) const {
    if (tid.ndims > 0) {
      tid = array_tid_;
    }

    const auto info = type_info_.find(tid);
    CHECK(info != type_info_.cend());
    return info->second;
  }

  string LookupTypeName(ast::TypeId tid) const;

  bool IsAncestor(ast::TypeId child, ast::TypeId ancestor) const;

  const map<ast::TypeId, TypeInfo>& GetTypeMap() const {
    return type_info_;
  }

private:
  friend class TypeInfoMapBuilder;

  using TypeMap = map<ast::TypeId, TypeInfo>;
  // TODO: Make safe for parallel compilation.
  using InheritMap = map<pair<ast::TypeId, ast::TypeId>, bool>;

  TypeInfoMap(const TypeMap& typeinfo, ast::TypeId array_tid) : type_info_(typeinfo), array_tid_(array_tid) {}

  bool IsAncestorRec(ast::TypeId child, ast::TypeId ancestor) const;

  static TypeInfoMap kEmptyTypeInfoMap;
  static TypeInfo kErrorTypeInfo;

  TypeMap type_info_;
  ast::TypeId array_tid_;
  mutable InheritMap inherit_map_;
};

class TypeInfoMapBuilder {
public:
  TypeInfoMapBuilder(ast::TypeId object_tid, ast::TypeId serializable_tid, ast::TypeId cloneable_tid);

  void PutType(ast::TypeId tid, const ast::ModifierList& mods, ast::TypeKind kind, const string& name, const string& package, base::PosRange pos, const vector<ast::TypeId>& extends, const vector<ast::TypeId>& implements) {
    type_entries_.push_back(TypeInfo{mods, kind, tid, name, package, pos, TypeIdList(extends), TypeIdList(implements), MethodTable::kEmptyMethodTable, FieldTable::kEmptyFieldTable, tid.base});
  }

  void PutType(ast::TypeId tid, const ast::TypeDecl& type, const string& package, const vector<ast::TypeId>& extends, const vector<ast::TypeId>& implements) {
    CHECK(tid.ndims == 0);
    PutType(tid, type.Mods(), type.Kind(), type.Name(), package, type.NameToken().pos, extends, implements);
  }

  void PutMethod(ast::TypeId curtid, const MethodInfo& minfo) {
    method_entries_.insert({curtid, minfo});
  }

  void PutMethod(ast::TypeId curtid, ast::TypeId rettid, const vector<ast::TypeId>& paramtids, const ast::MemberDecl& meth, bool is_constructor) {
    PutMethod(curtid, MethodInfo{kErrorMethodId, curtid, meth.Mods(), rettid, meth.NameToken().pos, MethodSignature{is_constructor, meth.Name(), TypeIdList(paramtids)}});
  }

  void PutField(ast::TypeId curtid, const FieldInfo& finfo) {
    field_entries_.insert({curtid, finfo});
  }

  void PutField(ast::TypeId curtid, ast::TypeId tid, const ast::MemberDecl& field) {
    PutField(curtid, FieldInfo{kErrorFieldId, curtid, field.Mods(), tid, field.NameToken().pos, field.Name()});
  }

  TypeInfoMap Build(base::ErrorList* out);

private:
  using MInfoIter = vector<MethodInfo>::iterator;
  using MInfoCIter = vector<MethodInfo>::const_iterator;
  using FInfoIter = vector<FieldInfo>::iterator;
  using FInfoCIter = vector<FieldInfo>::const_iterator;

  MethodTable MakeResolvedMethodTable(TypeInfo* tinfo, const MethodTable::MethodSignatureMap& good_methods, const set<string>& bad_methods, bool has_bad_constructor, const map<ast::TypeId, TypeInfo>& sofar, const set<ast::TypeId>& bad_types, set<ast::TypeId>* new_bad_types, base::ErrorList* out);

  void BuildMethodTable(MInfoIter begin, MInfoIter end, TypeInfo* tinfo, MethodId* cur_mid, const map<ast::TypeId, TypeInfo>& sofar, const set<ast::TypeId>& bad_types, set<ast::TypeId>* new_bad_types, base::ErrorList* out);

  void BuildFieldTable(FInfoIter begin, FInfoIter end, TypeInfo* tinfo, FieldId* cur_fid, const map<ast::TypeId, TypeInfo>& sofar, base::ErrorList* out);

  void ValidateExtendsImplementsGraph(map<ast::TypeId, TypeInfo>* m, set<ast::TypeId>* bad, base::ErrorList* errors);
  void PruneInvalidGraphEdges(const map<ast::TypeId, TypeInfo>&, set<ast::TypeId>*, base::ErrorList*);
  void IntroduceImplicitGraphEdges(const set<ast::TypeId>& bad, map<ast::TypeId, TypeInfo>* types);
  vector<ast::TypeId> VerifyAcyclicGraph(const multimap<ast::TypeId, ast::TypeId>&, set<ast::TypeId>*, std::function<void(const vector<ast::TypeId>&)>);

  base::Error* MakeConstructorNameError(base::PosRange pos) const;
  base::Error* MakeParentFinalError(const TypeInfo& minfo, const TypeInfo& pinfo) const;
  base::Error* MakeDifferingReturnTypeError(const TypeInfo& mtinfo, const MethodInfo& mminfo, const MethodInfo& pminfo) const;
  base::Error* MakeStaticMethodOverrideError(const TypeInfo& mtinfo, const MethodInfo& mminfo, const MethodInfo& pminfo) const;
  base::Error* MakeLowerVisibilityError(const TypeInfo& mtinfo, const MethodInfo& mminfo, const MethodInfo& pminfo) const;
  base::Error* MakeOverrideFinalMethodError(const MethodInfo& minfo, const MethodInfo& pinfo) const;
  base::Error* MakeParentClassEmptyConstructorError(const TypeInfo& minfo, const TypeInfo& pinfo) const;
  base::Error* MakeNeedAbstractClassError(const TypeInfo& tinfo, const MethodTable::MethodSignatureMap& method_map) const;

  base::Error* MakeExtendsCycleError(const vector<TypeInfo>& cycle) const;

  ast::TypeId object_tid_;
  ast::TypeId array_tid_;
  vector<TypeInfo> type_entries_;
  multimap<ast::TypeId, MethodInfo> method_entries_;
  multimap<ast::TypeId, FieldInfo> field_entries_;
};

} // namespace types

#endif
