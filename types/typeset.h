#ifndef TYPES_TYPESET_H
#define TYPES_TYPESET_H

#include <map>

#include "base/errorlist.h"
#include "base/file.h"
#include "base/fileset.h"
#include "ast/ast.h"
#include "ast/ids.h"

namespace types {

class TypeSet {
public:
  // Provides a `view' into the TypeSet assuming the provided imports are in
  // scope. Note that these do not `stack'; i.e.
  // `a.WithImports(bimports).WithImports(cimports)' is equivalent to
  // `a.WithImports(cimports)' regardless of the values of `bimports' and
  // `cimports'.
  TypeSet WithImports(const vector<ast::ImportDecl>& imports, const base::FileSet* fs, base::ErrorList* errors) const;

  // Returns a TypeId corresponding to the entire provided qualified name. If
  // no such type exists, then kErrorTypeId will be returned.
  ast::TypeId Get(const vector<string>& qualifiedname) const;

  // Returns a TypeId corresponding to a prefix of the provided qualfied name.
  // Will write the length of the prefix to *typelen. If no such type exists,
  // then kErrorTypeId will be returned.
  ast::TypeId GetPrefix(const vector<string>& qualifiedname, u64* typelen) const;

  void PrintTo(std::ostream* out) const {
    for (const auto& name : available_names_) {
      *out << name.first << "->" << name.second << '\n';
    }
  }

 private:
  friend class TypeSetBuilder;
  using QualifiedNameBaseMap = std::map<string, ast::TypeId::Base>;

  TypeSet() = default;
  TypeSet(const vector<string>& qualifiedTypes);

  static void InsertName(QualifiedNameBaseMap* m, string name, ast::TypeId::Base base);

  void InsertImport(const ast::ImportDecl& import, const base::FileSet* fs, base::ErrorList* errors);
  void InsertWildcardImport(const string& base);

  // Changed depending on provided Imports.
  QualifiedNameBaseMap available_names_;

  // Always kept identical to values from Builder.
  QualifiedNameBaseMap original_names_;
};

class TypeSetBuilder {
 public:
  // Automatically inserts 'int', 'short', 'byte', 'boolean', 'void', and 'error'.
  TypeSetBuilder() = default;

  // Add a type definition to this TypeSetBuilder. The namepos will be returned
  // from Build on duplicate definitions.
  void Put(const vector<string>& ns, const string& name, base::PosRange namepos);

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
