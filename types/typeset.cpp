#include "types/typeset.h"

#include <algorithm>
#include <iostream>
#include <iterator>

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
using base::MakeError;
using base::OutputOptions;
using base::Pos;
using base::PosRange;

namespace types {

namespace {

Error* MakeDuplicateTypeDefinitionError(const FileSet* fs, const string& name, const vector<PosRange> dupes) {
  return MakeError([=](ostream* out, const OutputOptions& opt) {
    if (opt.simple) {
      *out << name << ": [";
      for (const auto& dupe : dupes) {
        *out << dupe << ",";
      }
      *out << ']';
      return;
    }

    stringstream msgstream;
    msgstream << "Type '" << name << "' was declared multiple times.";

    PrintDiagnosticHeader(out, opt, fs, dupes.at(0), DiagnosticClass::ERROR, msgstream.str());
    PrintRangePtr(out, opt, fs, dupes.at(0));
    for (uint i = 1; i < dupes.size(); ++i) {
      *out << '\n';
      PosRange pos = dupes.at(i);
      PrintDiagnosticHeader(out, opt, fs, pos, DiagnosticClass::INFO, "Also declared here.");
      PrintRangePtr(out, opt, fs, pos);
    }
  });
}

} // namespace

TypeSet TypeSet::kEmptyTypeSet(sptr<TypeSetImpl>(new TypeSetImpl(&FileSet::Empty(), {}, {})));

void TypeSetBuilder::Put(const vector<string>& ns, const string& name, base::PosRange namepos) {
  stringstream ss;
  for (const auto& n : ns) {
    assert(n.find('.') == string::npos);
    ss << n << '.';
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
    NameToPosMap byname;
    for (const auto& entry : entries) {
      byname.insert({entry.name, entry.namepos});
    }

    auto cur = byname.cbegin();
    while (cur != byname.cend()) {
      // The range [start, end) is meant to track a range of duplicate entries.
      auto start = cur;
      auto end = std::next(start);
      int ndups = 1;

      while (end != byname.cend() && start->first == end->first) {
        ++ndups;
        ++end;
      }

      // Immediately advance our outer iterator past the relevant range, so
      // that we can't forget to do it later.
      cur = end;

      if (ndups == 1) {
        types.insert(start->first);
        continue;
      }
      assert(ndups > 1);

      vector<PosRange> defs;
      for (auto dup = start; dup != end; ++dup) {
        defs.push_back(dup->second);
      }

      out->Append(MakeDuplicateTypeDefinitionError(fs, start->first, defs));
      bad_types.insert(start->first);
    }
  }

  // TODO: Check other restrictions, like having a type be a proper prefix of a
  // package.

  return TypeSet(make_shared<TypeSetImpl>(fs, types, bad_types));
}

} // namespace types
