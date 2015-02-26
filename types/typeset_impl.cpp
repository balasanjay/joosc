#include "types/typeset_impl.h"

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

} // namespace

TypeSetImpl::TypeSetImpl(const FileSet* fs, const set<string>& types, const set<string>& bad_types) : fs_(fs) {
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

TypeId TypeSetImpl::Get(const vector<string>& qualifiedname) const {
  u64 typelen = -1;
  TypeId ret = GetPrefix(qualifiedname, &typelen);
  if ((uint)typelen < qualifiedname.size()) {
    return TypeId::kUnassigned;
  }
  return ret;
}

TypeId TypeSetImpl::GetPrefix(const vector<string>& qualifiedname, u64* typelen) const {
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

void TypeSetImpl::InsertName(QualifiedNameBaseMap* m, string name, TypeId::Base base) {
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


sptr<TypeSetImpl> TypeSetImpl::WithImports(const vector<ast::ImportDecl>& imports, ErrorList* errors) const {
  TypeSetImpl view(*this);

  view.InsertWildcardImport("java.lang");
  for (const auto& import : imports) {
    if (import.IsWildCard()) {
      view.InsertWildcardImport(import.Name().Name());
    } else {
      view.InsertImport(import, errors);
    }
  }

  return make_shared<TypeSetImpl>(std::move(view));
}

void TypeSetImpl::InsertImport(const ImportDecl& import, ErrorList* errors) {
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

void TypeSetImpl::InsertWildcardImport(const string& base) {
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
