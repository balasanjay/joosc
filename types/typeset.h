#ifndef TYPES_TYPESET_H
#define TYPES_TYPESET_H

#include <map>

#include "ast/ast.h"
#include "ast/ids.h"
#include "base/errorlist.h"
#include "base/file.h"
#include "base/fileset.h"
#include "types/typeset_impl.h"

namespace types {

class TypeSet {
public:
  TypeSet(const TypeSet&) = default;

  static const TypeSet& Empty() {
    return kEmptyTypeSet;
  }

  // Enter the root package. Mutually exclusive with WithPackage.
  TypeSet WithRootPackage() {
    return TypeSet(impl_->WithRootPackage());
  }

  // Enter a package. Mutually exclusive with WithRootPackage.
  TypeSet WithPackage(const vector<string>& package, base::PosRange pos) {
    return TypeSet(impl_->WithPackage(package, pos));
  }

  // Provides a `view' into the TypeSet assuming the provided imports are in
  // scope. Note that these do not `stack'; i.e.
  // `a.WithImports(bimports).WithImports(cimports)' is equivalent to
  // `a.WithImports(cimports)' regardless of the values of `bimports' and
  // `cimports'.
  TypeSet WithImports(const vector<ast::ImportDecl>& imports, base::ErrorList* errors) const {
    return TypeSet(impl_->WithImports(imports, errors));
  }

  // Returns a TypeId corresponding to a prefix of the provided qualified name.
  //
  // If the TypeSet has never seen any prefix of name before that it will
  // return TypeId::kUnassigned.
  //
  // If the TypeSet has already emitted an error about a type that is a prefix
  // of na,e, then it might return TypeId::kError. Clients should avoid
  // emitting further errors involving this type. Note that this is done
  // best-effort; the TypeSet might still return TypeId::kUnassigned instead.
  //
  // Otherwise, a valid TypeId will be returned, and the length of the prefix
  // will be written to *typelen.
  ast::TypeId GetPrefix(const vector<string>& qualifiedname, u64* typelen) const {
    return impl_->GetPrefix(qualifiedname, typelen);
  }

  // Helper method that calls GetPrefix, and filters out any results that don't
  // use all of qualifiedname. Any result from GetPrefix that doesn't encompass
  // the whole string will be turned into TypeId::kUnassigned.
  ast::TypeId Get(const vector<string>& qualifiedname) const {
    return impl_->Get(qualifiedname);
  }

 private:
  friend class TypeSetBuilder;

  TypeSet(sptr<const TypeSetImpl> impl) : impl_(impl) {}

  static TypeSet kEmptyTypeSet;

  sptr<const TypeSetImpl> impl_;
};

class TypeSetBuilder {
 public:
  // Automatically inserts 'int', 'short', 'byte', 'boolean', 'void', and 'error'.
  TypeSetBuilder() = default;

  // Add a package to this TypeSetBuilder. The namepos will be used to
  // construct Errors during Build.
  void AddPackage(const vector<string>& pkg, const vector<base::PosRange> pkgpos);

  // Add a type definition to this TypeSetBuilder. The namepos will be used to
  // construct Errors during Build.
  void AddType(const vector<string>& pkg, const vector<base::PosRange> pkgpos, const string& name, base::PosRange namepos);

  // Returns a TypeSet with all types inserted with Put. If a type was defined
  // multiple times, an Error will be appended to the ErrorList for each
  // duplicate location.
  TypeSet Build(const base::FileSet* fs, base::ErrorList* out) const;

 private:
  struct Entry {
    string name; // com.foo.Bar
    base::PosRange namepos;
  };
  vector<Entry> entries_;
};

} // namespace types

#endif
