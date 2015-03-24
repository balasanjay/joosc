#ifndef TYPES_TYPESET_H
#define TYPES_TYPESET_H

#include <map>

#include "ast/ast.h"
#include "ast/ids.h"
#include "base/errorlist.h"

namespace types {

class TypeSet {
public:
  TypeSet(const TypeSet&) = default;
  ~TypeSet() = default;

  static TypeSet Empty() {
    Data* data = new Data{
      {},
      {},
      {},
      {},
      {},
      {},
    };
    return TypeSet(sptr<Data>(data));
  }

  // Provides a `view' into this TypeSet assuming that lookups occur inside the
  // provided file.
  TypeSet WithCompUnit(int fileid) const {
    CHECK(fid_ == -1);
    return TypeSet(data_, fileid, data_->file_pkg_index_.at(fileid));
  }

  // Resolve a type name in the current environment.
  ast::TypeId Get(const string& name, base::PosRange pos, base::ErrorList* errors) const;

  ast::TypeId TryGet(const string& name) const {
    static const base::PosRange kFakePos(-1, -1, -1);
    base::ErrorList throwaway;
    return Get(name, kFakePos, &throwaway);
  }

private:
  friend class TypeSetBuilder;

  using TypeBase = ast::TypeId::Base;

  struct Type {
    // The simple name of the type; i.e. "String", for the stdlib type
    // "java.lang.String".
    string simple_name;

    // The fully qualified name of the type; i.e. "java.lang.String".
    string longname;

    // "java.lang". The empty string if this type is in the unnamed package.
    string pkg;

    // The position of this type's declaration.
    //
    // Note that this may refer to both a type's actual declaration, or its
    // import location. Both of these are declarations, of a sort.
    base::PosRange pos;

    // The base of the TypeId of this Type.
    TypeBase tid;
  };

  struct WildCardImport {
    // The file in which this wildcard import occurs.
    int fid;

    // The package being wildcard-imported.
    string pkg;

    // The position of the wildcard import declaration.
    base::PosRange pos;
  };

  // To avoid copies, all TypeSets share a pointer to a single Data struct.
  struct Data {
    // An index supporting lookup of types in compilation unit scope. In other
    // words, if compilation unit foo.java declares type foo and imports type
    // bar, then this vector will have two entries corresponding to these two
    // facts.
    //
    // It follows that types might be present several times in this list,
    // corresponding to all their single-import-declarations as well as their
    // actual full declaration.
    //
    // Sorted first by file id, then by name. It is guaranteed that for a given
    // (file id, name) pair, there is exactly 0 or 1 corresponding type in this
    // vector. Duplicates are pruned early, and are replaced with blacklist
    // entries.
    vector<Type> comp_unit_scope_;

    // An index supporting lookup of types in package scope. In other words, if
    // package foo contains types bar and baz, then this vector will have two
    // entries corresponding to those two facts.
    //
    // It follows that each type only appears once, as it can only be in a
    // single package.
    //
    // Sorted first by package, then by name. It is guaranteed that for a given
    // (pkg, name) pair, there is exactly 0 or 1 corresponding type in the
    // vector.
    vector<Type> pkg_scope_;

    // An index supporting lookup of all wildcard imports in a compilation
    // unit. In other words, if a compilation unit imports "com.google.*", and
    // "java.lang.*", then this vector will contain two entries corresponding
    // to these two facts.
    //
    // Sorted by fileid, then by name. It is guaranteed that duplicate wildcard
    // imports have been collapsed into a single entry in this vector.
    vector<WildCardImport> wildcard_imports_;

    // A cache for wildcard-import lookups.
    // Maps (fileid, simple name) to TypeId:Base.
    mutable map<pair<int, string>, TypeBase> wildcard_lookup_cache_;

    // An index supporting lookup of types by qualified name.
    //
    // Sorted by qualified name. It is guaranteed that there is only a single
    // entry with a given qualified name.
    vector<Type> qual_name_index_;

    // An index supporting lookup of package given a file id.
    map<int, string> file_pkg_index_;
  };

  TypeSet(sptr<const Data> data, int fid, const string& pkg) : data_(data), fid_(fid), pkg_(pkg) {
    CHECK(data_ != nullptr);
  }

  TypeSet(sptr<const Data> data) : TypeSet(data, -1, "") {}

  const Type* LookupInPkgScope(const string& pkg, const string& name) const;

  sptr<const Data> data_;
  int fid_ = -1;
  string pkg_ = "";
};

class TypeSetBuilder {
public:
  TypeSetBuilder() = default;

  void AddCompUnit(sptr<const ast::CompUnit> unit) {
    units_.push_back(unit);
  }

  TypeSet Build(base::ErrorList* out) const;

private:
  using Type = TypeSet::Type;
  using WildCardImport = TypeSet::WildCardImport;
  using TypeBase = TypeSet::TypeBase;
  using Data = TypeSet::Data;

  void ExtractTypesAndPackages(
      vector<Type>*,
      map<string, base::PosRange>*,
      map<int, string>* file_to_pkg) const;

  void CheckTypePackageCollision(
      const map<string, base::PosRange>& all_pkgs,
      vector<Type>* types,
      base::ErrorList* errors) const;

  void BuildQualifiedNameIndex(
      vector<Type>* all_types,
      vector<Type>* qual_name_index,
      base::ErrorList* errors) const;

  void ResolveImports(
      const map<string, base::PosRange>& all_pkgs,
      const vector<Type>& qual_name_index,
      vector<Type>* comp_unit_scope,
      vector<WildCardImport>* wildcards,
      base::ErrorList* errors) const;

  vector<Type> ResolvePkgScope(const vector<Type>& all_types) const;

  vector<sptr<const ast::CompUnit>> units_;
};

} // namespace types

#endif
