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

TypeSet TypeSet::kEmptyTypeSet(sptr<TypeSetImpl>(new TypeSetImpl(&FileSet::Empty(), {}, {})));

void TypeSetBuilder::AddPackage(const vector<string>&, const vector<PosRange>) {
  // TODO: implement me.
}

void TypeSetBuilder::AddType(const vector<string>& pkg, const vector<PosRange>, const string& name, base::PosRange namepos) {
  stringstream ss;
  for (const auto& p : pkg) {
    assert(p.find('.') == string::npos);
    ss << p << '.';
  }
  assert(name.find('.') == string::npos);
  ss << name;
  entries_.push_back(Entry{ss.str(), namepos});
}

TypeSet TypeSetBuilder::Build(const FileSet* fs, base::ErrorList* out) const {
  vector<Entry> entries(entries_);

  set<string> types;
  set<string> bad_types;

  // First, we identify and strip out any duplicates.
  {
    using NameToPosMap = multimap<string, PosRange>;
    using NamePos = pair<string, PosRange>;
    using Iter = NameToPosMap::const_iterator;
    NameToPosMap byname;
    for (const auto& entry : entries) {
      byname.insert({entry.name, entry.namepos});
    }

    auto cmp = [](const NamePos& lhs, const NamePos& rhs) {
      return lhs.first == rhs.first;
    };

    auto cb = [&](Iter start, Iter end, i64 ndups) {
      if (ndups == 1) {
        types.insert(start->first);
        return;
      }
      assert(ndups > 1);

      vector<PosRange> defs;
      for (auto cur = start; cur != end; ++cur) {
        defs.push_back(cur->second);
      }

      assert(defs.size() == (size_t)ndups);

      stringstream msgstream;
      msgstream << "Type '" << start->first << "' was declared multiple times.";
      out->Append(MakeDuplicateDefinitionError(fs, defs, msgstream.str(), start->first));
      bad_types.insert(start->first);
    };

    FindEqualRanges(byname.cbegin(), byname.cend(), cmp, cb);
  }

  // TODO: Check other restrictions, like having a type be a proper prefix of a
  // package.

  return TypeSet(make_shared<TypeSetImpl>(fs, types, bad_types));
}

} // namespace types
