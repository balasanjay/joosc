#ifndef TYPES_TYPESET_H
#define TYPES_TYPESET_H

#include <map>

#include "ast/ast.h"
#include "ast/ids.h"
#include "base/errorlist.h"
#include "types/typeset_impl.h"

namespace types {

class TypeSet {
public:
  TypeSet(const TypeSet&) = default;

  static const TypeSet& Empty() {
    return kEmptyTypeSet;
  }

  // Enter a package. package can be nullptr, which means use the unnamed
  // package.
  TypeSet WithPackage(sptr<const ast::QualifiedName> package, base::ErrorList* errors) const {
    string pkg = "";
    base::PosRange pos(-1, -1, -1);
    if (package != nullptr) {
      pkg = package->Name();
      pos = package->Tokens().front().pos;
      pos.end = package->Tokens().back().pos.end;
    }
    return TypeSet(impl_->WithPackage(pkg, pos, errors));
  }

  // Provides a `view' into the TypeSet assuming the provided imports are in
  // scope.
  TypeSet WithImports(const vector<ast::ImportDecl>& imports, base::ErrorList* errors) const {
    return TypeSet(impl_->WithImports(imports, errors));
  }

  // Enter a type declaration. Will validate that the named type does not
  // conflict with an import in the same file.
  TypeSet WithType(const string& name, base::PosRange pos, base::ErrorList* errors) const {
    return TypeSet(impl_->WithType(name, pos, errors));
  }

  ast::TypeId Get(const string& name, base::PosRange pos, base::ErrorList* errors) const {
    return impl_->Get(name, pos, errors);
  }
  ast::TypeId TryGet(const string& name) const {
    static const base::PosRange fakepos(-1, -1, -1);
    base::ErrorList throwaway;
    return impl_->Get(name, fakepos, &throwaway);
  }
 private:
  friend class TypeSetBuilder;

  TypeSet(sptr<const TypeSetImpl> impl) : impl_(impl) {}

  static TypeSet kEmptyTypeSet;

  sptr<const TypeSetImpl> impl_;
};

class TypeSetBuilder {
 public:
  // pair<string, PosRange> with better field naming.
  struct Elem {
    string name;
    base::PosRange pos;
  };

  // Automatically inserts 'int', 'short', 'byte', 'boolean', 'void', and 'error'.
  TypeSetBuilder() = default;

  // Add a package to this TypeSetBuilder. The namepos will be used to
  // construct Errors during Build.
  void AddPackage(const vector<Elem>& pkg) {
    if (pkg.size() == 0) {
      pkgs_.insert({TypeSetImpl::kUnnamedPkgPrefix, base::PosRange(-1, -1, -1)});
      return;
    }

    stringstream ss;
    ss << TypeSetImpl::kNamedPkgPrefix;
    for (uint i = 0; i < pkg.size(); ++i) {
      const Elem& elem = pkg.at(i);
      ss << '.' << elem.name;

      // Inserts without replacing, so we keep the first pos we saw for a package.
      pkgs_.insert({ss.str(), elem.pos});
    }
  }

  // Add a type definition to this TypeSetBuilder.
  void AddType(const vector<Elem>& pkg, const Elem& type) {
    AddPackage(pkg);
    types_.push_back(Entry{pkg, type});
  }

  // Returns a TypeSet with all types inserted with Add*. If a type was defined
  // multiple times, an Error will be appended to the ErrorList for each
  // duplicate location.
  TypeSet Build(base::ErrorList* out) const;

 private:
  struct Entry {
    vector<Elem> pkg;
    Elem type;
  };

  string FullyQualifiedTypeName(const Entry& entry) const;

  vector<Entry> types_;

  // Records all seen packages. Keeps a single PosRange, so that we can point
  // any relevant errors to that PosRange.
  map<string, base::PosRange> pkgs_;
};

class TypeSet2 {
public:
  TypeSet2(const TypeSet2&) = default;
  ~TypeSet2() = default;

  static TypeSet2 Empty() {
    Data* data = new Data{
      {},
      {},
      {},
      {},
      {},
      {},
    };
    return TypeSet2(sptr<Data>(data));
  }

  // Provides a `view' into this TypeSet assuming that lookups occur inside the
  // provided file.
  TypeSet2 WithCompUnit(int fileid) const {
    CHECK(fid_ == -1);
    return TypeSet2(data_, fileid, data_->file_pkg_index_.at(fileid));
  }

  // Resolve a type name in the current environment.
  ast::TypeId Get(const string& name, base::PosRange pos, base::ErrorList* errors) const;

  ast::TypeId TryGet(const string& name) const {
    static const base::PosRange kFakePos(-1, -1, -1);
    base::ErrorList throwaway;
    return Get(name, kFakePos, &throwaway);
  }

private:
  friend class TypeSetBuilder2;

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
    // vector. Duplicates are pruned early, and are replaced with blaclist
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

  TypeSet2(sptr<const Data> data, int fid, const string& pkg) : data_(data), fid_(fid), pkg_(pkg) {
    CHECK(data_ != nullptr);
  }

  TypeSet2(sptr<const Data> data) : TypeSet2(data, -1, "") {}

  const Type* LookupInPkgScope(const string& pkg, const string& name) const;

  sptr<const Data> data_;
  int fid_ = -1;
  string pkg_ = "";
};

class TypeSetBuilder2 {
public:
  TypeSetBuilder2() = default;

  void AddCompUnit(sptr<const ast::CompUnit> unit) {
    units_.push_back(unit);
  }

  TypeSet2 Build(base::ErrorList* out) const;

private:
  using Type = TypeSet2::Type;
  using WildCardImport = TypeSet2::WildCardImport;
  using TypeBase = TypeSet2::TypeBase;
  using Data = TypeSet2::Data;

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
  vector<Type> MakeSimpleNameIndex(const vector<Type>& all_types) const;

  vector<sptr<const ast::CompUnit>> units_;
};

} // namespace types

#endif
