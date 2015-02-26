#ifndef TYPES_TYPESET_IMPL_H
#define TYPES_TYPESET_IMPL_H

#include <map>

#include "base/errorlist.h"
#include "base/file.h"
#include "base/fileset.h"
#include "ast/ast.h"
#include "ast/ids.h"

namespace types {

class TypeSetImpl {
public:
  TypeSetImpl(const base::FileSet* fs, const set<string>& types, const set<string>& bad_types);

  sptr<TypeSetImpl> WithRootPackage() const;
  sptr<TypeSetImpl> WithPackage(const vector<string>& package, base::PosRange pos) const;
  sptr<TypeSetImpl> WithImports(const vector<ast::ImportDecl>& imports, base::ErrorList* errors) const;

  ast::TypeId GetPrefix(const vector<string>& qualifiedname, u64* typelen) const;
  ast::TypeId Get(const vector<string>& qualifiedname) const;

  void PrintTo(std::ostream* out) const {
    for (const auto& name : available_names_) {
      *out << name.first << "->" << name.second << '\n';
    }
  }

  void InsertImport(const ast::ImportDecl& import, base::ErrorList* errors);
  void InsertWildcardImport(const string& base);

 private:
  using QualifiedNameBaseMap = map<string, ast::TypeId::Base>;

  static void InsertName(QualifiedNameBaseMap* m, string name, ast::TypeId::Base base);

  const base::FileSet* fs_;

  // Changed depending on provided Imports.
  QualifiedNameBaseMap available_names_;

  // Always kept identical to values from Builder.
  QualifiedNameBaseMap original_names_;
};

} // namespace types

#endif
