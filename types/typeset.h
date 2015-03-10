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

} // namespace types

#endif
