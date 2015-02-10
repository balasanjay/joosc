#ifndef TYPES_TYPES_H
#define TYPES_TYPES_H

#include "base/file.h"
#include "parser/ast.h"
#include <map>

namespace types {

struct TypeId {
  typedef u64 Base;

  Base base;
  u64 ndims;
};

const TypeId kUnassignedTypeId = {0, 0};
const u64 kErrorTypeIdBase = 1;

class TypeSet {
public:
  // Provides a `view' into the TypeSet assuming the provided imports are in
  // scope. Note that these do not `stack'; i.e.
  // `a.WithImports(bimports).WithImports(cimports)' is equivalent to
  // `a.WithImports(cimports)' regardless of the values of `bimports' and
  // `cimports'.
  TypeSet WithImports(const vector<parser::ImportDecl>& imports) const;

  // Returns a TypeId corresponding to the entire provided qualified name. If
  // no such type exists, then kErrorTypeId will be returned.
  TypeId Get(const vector<string>& qualifiedname) const;

  // Returns a TypeId corresponding to a prefix of the provided qualfied name.
  // Will write the length of the prefix to *typelen. If no such type exists,
  // then kErrorTypeId will be returned.
  TypeId GetPrefix(const vector<string>& qualifiedname, u64* typelen) const;

  void PrintTo(std::ostream* out) {
    for (const auto& name : available_names_) {
      *out << name.first << "->" << name.second << '\n';
    }
  }

  // TODO: make private.
  TypeSet(const vector<string>& qualifiedTypes);

 private:
  friend class TypeSetBuilder;
  typedef std::map<string, TypeId::Base> QualifiedNameBaseMap;

  TypeSet() = default;

  static void InsertName(QualifiedNameBaseMap* m, string name, TypeId::Base base);

  void InsertImport(const parser::ImportDecl& import);
  void InsertWildcardImport(const string& base);

  // Changed depending on provided Imports.
  QualifiedNameBaseMap available_names_;

  // Always kept identical to values from Builder.
  QualifiedNameBaseMap original_names_;
};

class TypeSetBuilder {
  // Automatically inserts 'int', 'short', 'byte', 'boolean', 'void', and 'error'.
  TypeSetBuilder() = default;

  // Add a type definition to this TypeSetBuilder. The namepos will be returned
  // from Build on duplicate definitions.
  void Put(const vector<string>& ns, const string& name, base::PosRange namepos);

  // Returns a TypeSet with all types inserted with Put. If a type was defined
  // multiple times, a vector containing all definitions' PosRanges will be
  // appended to *duplicates.
  TypeSet Build(vector<vector<base::PosRange>>* duplicates) const;

  private:
};

} // namespace types

#endif
