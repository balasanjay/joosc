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
  enum class ImportScope {
    // Compilation-unit scoped imports include single-import-statements, and
    // all the types declared inside the compilation unit. No conflicts are
    // allowed at this import scope.
    COMP_UNIT = 0,

    // Package scope lets you name all types that are in the same compilation
    // unit as you. No conflicts are allowed at this import scope (this is
    // handled by checking the type-uniqueness constraint).
    PACKAGE = 1,

    // Wildcard scope includes all wildcard-import-statements. Conflicts ARE
    // allowed at this import scope, and errors are only emitted upon use.
    WILDCARD = 2,
  };

  TypeSetImpl(const base::FileSet* fs, const map<string, base::PosRange>& types, const map<string, base::PosRange>& pkgs, const set<string>& bad_types);

  // See TypeSet for docs.
  sptr<TypeSetImpl> WithPackage(const string&, base::PosRange, base::ErrorList*) const;
  sptr<TypeSetImpl> WithImports(const vector<ast::ImportDecl>&, base::ErrorList*) const;
  sptr<TypeSetImpl> WithType(const string& name, base::PosRange pos, base::ErrorList*) const;
  ast::TypeId Get(const string&, base::PosRange, base::ErrorList*) const;

  void PrintTo(std::ostream* out) const {
    for (const auto& name : visible_types_) {
      *out << name.shortname << "->" << name.base << "(" <<
        (int)name.scope << ")" << '\n';
    }
  }

  static const int kPkgPrefixLen;
  static const string kUnnamedPkgPrefix;
  static const string kNamedPkgPrefix;

  void InsertAtScope(ImportScope scope, const string& longname, base::PosRange pos, base::ErrorList* out);

  void InsertWildCard(ImportScope scope, const string& basename, base::PosRange pos, base::ErrorList* out);

 private:
  using TypeBase = ast::TypeId::Base;

  struct TypeInfo {
    // These may be considered for sorting.
    string shortname;
    TypeBase base;
    ImportScope scope;

    // These are auxilary attributes that are not consulted for sorting.
    string longname;
    base::PosRange pos;

    struct ByName {
      bool operator()(const TypeInfo& lhs, const TypeInfo& rhs) const {
        return std::tie(lhs.shortname, lhs.base, lhs.scope) < std::tie(rhs.shortname, rhs.base, rhs.scope);
      }
    };
  };

  using VisibleSet = set<TypeInfo, TypeInfo::ByName>;

  pair<VisibleSet::const_iterator, VisibleSet::const_iterator> FindByShortName(const string& shortname) const;

  const base::FileSet* fs_;

  VisibleSet visible_types_;

  // All declared types. Kept immutable after recieving from Builder.
  map<string, pair<TypeBase, base::PosRange>> types_;

  // All declared packages. Kept immutable after recieving from Builder.
  map<string, base::PosRange> pkgs_;

  string pkg_prefix_ = "";
};

} // namespace types

#endif
