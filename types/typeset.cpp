#include "types/typeset.h"

#include <algorithm>
#include <iostream>
#include <iterator>

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

TypeSet TypeSet::kEmptyTypeSet = TypeSet();

TypeSet::TypeSet(const vector<string>& qualifiedTypes) {
  // Get list of all types.
  vector<string> predefs = {
    "<unassigned>",
    "<error>",
    "<null>",
    "void",
    "boolean",
    "byte",
    "char",
    "short",
    "int",
  };

  u64 i = 0;
  for (const auto& predef : predefs) {
    original_names_[predef] = TypeId::Base(i);
    i++;
  }

  for (const auto& qualifiedType : qualifiedTypes) {
    original_names_[qualifiedType] = TypeId::Base(i);
    i++;
  }

  available_names_ = original_names_;
}

TypeId TypeSet::Get(const vector<string>& qualifiedname) const {
  u64 typelen = -1;
  TypeId ret = GetPrefix(qualifiedname, &typelen);
  if ((uint)typelen < qualifiedname.size() || ret.base == TypeId::kErrorBase) {
    return TypeId::Error();
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

  return TypeId::Error();
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


TypeSet TypeSet::WithImports(const vector<ast::ImportDecl>& imports, const FileSet* fs, ErrorList* errors) const {
  TypeSet view;
  view.original_names_ = original_names_;
  view.available_names_ = available_names_;

  view.InsertWildcardImport("java.lang");
  for (const auto& import : imports) {
    if (import.IsWildCard()) {
      view.InsertWildcardImport(import.Name().Name());
    } else {
      view.InsertImport(import, fs, errors);
    }
  }

  return view;
}

void TypeSet::InsertImport(const ImportDecl& import, const FileSet* fs, ErrorList* errors) {
  auto iter = original_names_.find(import.Name().Name());

  if (iter == original_names_.end()) {
    errors->Append(MakeUnknownImportError(fs, import.Name().Tokens().back().pos));
    return;
  }

  // TODO: errors.
  assert(iter != original_names_.end());
  TypeId::Base tid = iter->second;

  const vector<string>& parts = import.Name().Parts();
  const string& last = parts.at(parts.size() - 1);

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
  // Find duplicates.
  vector<Entry> entries(entries_);
  stable_sort(entries.begin(), entries.end(),
      [](const Entry& lhs, const Entry& rhs) { return lhs.name < rhs.name; });

  uint start = 0;
  uint end = 1;
  while (start < entries.size()) {
    if (end < entries.size() && entries[start].name == entries[end].name) {
      ++end;
      continue;
    }

    // Check for duplicate definitions.
    if (end - start > 1) {
      vector<PosRange> defs;
      for (uint i = start; i < end; ++i) {
        defs.push_back(entries.at(i).namepos);
      }
      out->Append(MakeDuplicateTypeDefinitionError(fs, entries[start].name, defs));
    }

    start = end;
    ++end;
  }

  vector<string> qualifiedNames;
  transform(entries.begin(), entries.end(), back_inserter(qualifiedNames),
      [](const Entry& e) { return e.name; });

  return TypeSet(qualifiedNames);
}

} // namespace types
