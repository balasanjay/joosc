#include "types/typeset.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include "base/algorithm.h"

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

namespace {

Error* MakeUnknownImportError(const FileSet* fs, PosRange pos) {
  return MakeSimplePosRangeError(fs, pos, "UnknownImportError",
                                 "Cannot find imported class.");
}

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

TypeSet TypeSet::kEmptyTypeSet(&FileSet::Empty(), {}, {});

TypeSet::TypeSet(const FileSet* fs, const set<string>& types, const set<string>& bad_types) : fs_(fs) {
  // Insert all predefined types.
  original_names_["void"] = TypeId::kVoidBase;
  original_names_["boolean"] = TypeId::kBoolBase;
  original_names_["byte"] = TypeId::kByteBase;
  original_names_["char"] = TypeId::kCharBase;
  original_names_["short"] = TypeId::kShortBase;
  original_names_["int"] = TypeId::kIntBase;

  u64 i = TypeId::kFirstRefTypeBase;
  for (const auto& type : types) {
    auto result = original_names_.insert(make_pair(type, TypeId::Base(i)));
    i++;

    // Verify that we didn't overwrite a key.
    assert(result.second);
  }

  for (const auto& bad_type : bad_types) {
    auto result = original_names_.insert(make_pair(bad_type, TypeId::kError.base));
    // Verify that we didn't overwrite a key.
    assert(result.second);
  }

  available_names_ = original_names_;
}

TypeId TypeSet::Get(const vector<string>& qualifiedname) const {
  u64 typelen = -1;
  TypeId ret = GetPrefix(qualifiedname, &typelen);
  if ((uint)typelen < qualifiedname.size()) {
    return TypeId::kUnassigned;
  }
  return ret;
}

TypeId TypeSet::GetPrefix(const vector<string>& qualifiedname, u64* typelen) const {
  assert(qualifiedname.size() > 0);

  string namestr;

  {
    stringstream ss;
    ss << qualifiedname[0];
    for (uint i = 1; i < qualifiedname.size(); ++i) {
      ss << '.' << qualifiedname.at(i);
    }
    namestr = ss.str();
  }

  for (uint i = 0; i < qualifiedname.size(); ++i) {
    uint len = qualifiedname.size() - i;
    if (i != 0) {
      namestr = namestr.substr(0, namestr.size() - qualifiedname[len - 1].size() - 1);
    }

    auto iter = available_names_.find(namestr);
    if (iter == available_names_.end()) {
      continue;
    }

    *typelen = len;
    return TypeId{iter->second, 0};
  }

  return TypeId::kUnassigned;
}

void TypeSet::InsertName(QualifiedNameBaseMap* m, string name, TypeId::Base base) {
  auto iter = m->find(name);

  // New import.
  if (iter == m->end()) {
    (*m)[name] = base;
    return;
  }

  // Already imported.
  if (iter->second == base) {
    return;
  }

  // TODO: handle name-clash.
  throw;
}


TypeSet TypeSet::WithImports(const vector<ast::ImportDecl>& imports, ErrorList* errors) const {
  TypeSet view(*this);

  view.InsertWildcardImport("java.lang");
  for (const auto& import : imports) {
    if (import.IsWildCard()) {
      view.InsertWildcardImport(import.Name().Name());
    } else {
      view.InsertImport(import, errors);
    }
  }

  return view;
}

void TypeSet::InsertImport(const ImportDecl& import, ErrorList* errors) {
  const string& full = import.Name().Name();
  const vector<string>& parts = import.Name().Parts();
  const string& last = parts.at(parts.size() - 1);

  auto iter = original_names_.find(full);

  if (iter == original_names_.end()) {
    errors->Append(MakeUnknownImportError(fs_, import.Name().Tokens().back().pos));

    // Add both versions of the name to blacklist, without overwriting any
    // existing entries.
    available_names_.insert(make_pair(full, TypeId::kError.base));
    available_names_.insert(make_pair(last, TypeId::kError.base));

    return;
  }

  // TODO: errors.
  assert(iter != original_names_.end());
  TypeId::Base tid = iter->second;
  InsertName(&available_names_, last, tid);
}

void TypeSet::InsertWildcardImport(const string& base) {
  auto prefix = base + ".";
  auto begin = original_names_.lower_bound(prefix);
  auto end = original_names_.end();

  for (auto iter = begin; iter != end; ++iter) {
    auto decl = iter->first;
    if (decl.size() < prefix.size() || count(decl.begin() + prefix.size(), decl.end(), '.') > 0) {
      continue;
    }

    if (decl.compare(0, prefix.size(), prefix) != 0) {
      break;
    }

    string key = decl.substr(prefix.size());
    InsertName(&available_names_, key, iter->second);
  }
}

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

      out->Append(MakeDuplicateTypeDefinitionError(fs, start->first, defs));
      bad_types.insert(start->first);
    };

    FindEqualRanges(byname.cbegin(), byname.cend(), cmp, cb);
  }

  // TODO: Check other restrictions, like having a type be a proper prefix of a
  // package.

  return TypeSet(fs, types, bad_types);
}

} // namespace types
