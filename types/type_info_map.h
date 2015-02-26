#ifndef TYPES_TYPE_INFO_MAP_H
#define TYPES_TYPE_INFO_MAP_H

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

  // TODO: implement operator <. See std::lexicographical_compare
private:
  vector<ast::TypeId> tids_;
};

using MethodId = u64;

enum CallContext {
  INSTANCE,
  CONSTRUCTOR,
  STATIC,
};

struct MethodInfo {
  ast::ModifierList mods;
  ast::TypeId returnType;
  string name;
  base::PosRange namePos;
  TypeIdList paramTypes;
};

struct TypeInfo {
  ast::ModifierList mods;
  ast::TypeKind kind;
  ast::TypeId type;
  string name;
  base::PosRange namePos;
  TypeIdList extends;
  TypeIdList implements;
};

class MethodTable {
public:
  MethodId ResolveCall(ast::TypeId callerType, CallContext ctx, const TypeIdList& params, base::ErrorList* out) const;

  const MethodInfo& LookupMethod(MethodId mid) const;
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

  TypeInfoMap Build(const base::FileSet* /*fs*/, base::ErrorList* /*out*/) {
    return TypeInfoMap();
  }
};

} // namespace types

#endif
