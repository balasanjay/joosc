#include "types/typeset.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include "base/algorithm.h"
#include "types/types_internal.h"
#include "types/typeset_impl.h"

using std::back_inserter;
using std::count;
using std::cout;
using std::ostream;
using std::sort;
using std::transform;

using ast::ImportDecl;
using ast::TypeId;
using base::DiagnosticClass;
using base::Error;
using base::ErrorList;
using base::FileSet;
using base::FindEqualRanges;
using base::MakeError;
using base::OutputOptions;
using base::Pos;
using base::PosRange;

namespace types {

TypeSet TypeSet::kEmptyTypeSet(sptr<TypeSetImpl>(new TypeSetImpl(&FileSet::Empty(), {}, {}, {})));

string TypeSetBuilder::FullyQualifiedTypeName(const Entry& entry) const {
  stringstream ss;
  if (entry.pkg.size() == 0) {
    ss << TypeSetImpl::kUnnamedPkgPrefix;
  } else {
    ss << TypeSetImpl::kNamedPkgPrefix;
  }
  for (const auto& part : entry.pkg) {
    ss << '.' << part.name;
  }
  ss << '.' << entry.type.name;
  return ss.str();
}

TypeSet TypeSetBuilder::Build(const FileSet* fs, base::ErrorList* out) const {
  using NamePos = pair<string, base::PosRange>;
  using NamePosMap = map<string, base::PosRange>;
  using NamePosMultiMap = multimap<string, base::PosRange>;

  NamePosMap declared_pkgs(pkgs_);

  // Get fully qualified name of all types.
  vector<Elem> declared_types;
  for (const auto& entry : types_) {
    declared_types.push_back(Elem{FullyQualifiedTypeName(entry), entry.type.pos});
  }

  // First, we identify and strip out any duplicates.
  set<string> bad_names;
  {
    using Iter = NamePosMultiMap::const_iterator;

    // Put both types and packages in a single multimap.
    NamePosMultiMap byname;
    for (const auto& type : declared_types) {
      byname.insert({type.name, type.pos});
    }
    for (const auto& pkg : declared_pkgs) {
      byname.insert({pkg.first, pkg.second});
    }

    auto cmp = [](const NamePos& lhs, const NamePos& rhs) {
      return lhs.first == rhs.first;
    };

    auto cb = [&](Iter start, Iter end, i64 ndups) {
      if (ndups == 1) {
        return;
      }
      CHECK(ndups > 1);

      vector<PosRange> defs;
      for (auto cur = start; cur != end; ++cur) {
        defs.push_back(cur->second);
      }

      CHECK(defs.size() == (size_t)ndups);

      string without_prefix = start->first.substr(TypeSetImpl::kPkgPrefixLen+1);
      stringstream msgstream;
      msgstream << "Type '" << without_prefix << "' was declared multiple times.";
      out->Append(MakeDuplicateDefinitionError(fs, defs, msgstream.str(), without_prefix));
      bad_names.insert(start->first);
    };

    FindEqualRanges(byname.cbegin(), byname.cend(), cmp, cb);
  }

  // Build final set of types and packages by filtering out blacklisted names.
  set<string> types;
  set<string> pkgs;

  // Helper that inserts a string into a set, if its not blacklisted.
  auto insert_if_not_bad = [](const string& name, const set<string>& bad, set<string>* out) {
    if (bad.count(name) == 1) {
      return;
    }
    auto iterok = out->insert(name);
    CHECK(iterok.second);
  };

  for (const auto& type : declared_types) {
    insert_if_not_bad(type.name, bad_names, &types);
  }
  for (const auto& pkg : declared_pkgs) {
    insert_if_not_bad(pkg.first, bad_names, &pkgs);
  }

  return TypeSet(make_shared<TypeSetImpl>(fs, types, pkgs, bad_names));
}

} // namespace types
