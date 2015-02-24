#ifndef TYPES_TYPE_INFO_MAP_H
#define TYPES_TYPE_INFO_MAP_H

#include "ast/ast.h"
#include "ast/ids.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace types {

class TypeInfoMap {
public:
  static const TypeInfoMap& Empty() {
    return kEmptyTypeInfoMap;
  }

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
