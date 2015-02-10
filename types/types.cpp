#include "types/types.h"
#include <algorithm>
#include <iostream>
#include <iterator>

using parser::ImportDecl;
using std::back_inserter;
using std::count;
using std::cout;
using std::sort;

namespace types {

TypeSet::TypeSet(const vector<string>& qualifiedTypes) {
  // Get list of all types.
  vector<string> predefs = {
    "!!unassigned!!",
    "!!error!!",
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
  if ((uint)typelen < qualifiedname.size() || ret.base == kErrorTypeIdBase) {
    return TypeId{kErrorTypeIdBase, 0};
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

  // Don't strip all segments, stop at second-last.
  for (uint i = 0; i < qualifiedname.size() - 1; ++i) {
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

  return TypeId{kErrorTypeIdBase, 0};
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
  assert(false);
}


TypeSet TypeSet::WithImports(const vector<parser::ImportDecl>& imports) const {
  TypeSet view;
  view.original_names_ = original_names_;
  view.available_names_ = available_names_;

  view.InsertWildcardImport("java.lang");
  for (const auto& import : imports) {
    if (import.IsWildCard()) {
      view.InsertWildcardImport(import.Name().Name());
    } else {
      view.InsertImport(import);
    }
  }

  return view;
}

void TypeSet::InsertImport(const ImportDecl& import) {
  auto iter = original_names_.find(import.Name().Name());

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

} // namespace types
