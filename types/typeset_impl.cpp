#include "types/typeset_impl.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include "types/types_internal.h"

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

Error* MakeAmbiguousTypeError(const FileSet* fs, PosRange pos, const string& msg) {
  return MakeSimplePosRangeError(fs, pos, "AmbiguousType", msg);
}

Error* MakeUnknownPackageError(const FileSet* fs, PosRange pos) {
  return MakeSimplePosRangeError(fs, pos, "UnknownPackageError",
                                 "Cannot find imported package.");
}

} // namespace

const int TypeSetImpl::kPkgPrefixLen = 3;
const string TypeSetImpl::kUnnamedPkgPrefix = "<0>";
const string TypeSetImpl::kNamedPkgPrefix = "<1>";

TypeSetImpl::TypeSetImpl(const FileSet* fs, const set<string>& types, const set<string>& pkgs, const set<string>& bad_types) : fs_(fs), pkgs_(pkgs) {
  // Insert all predefined types.
#define INS(name, Name) \
  types_[#name] = TypeId::k##Name##Base; \
  visible_types_.insert({#name, TypeInfo{#name, TypeId::k##Name##Base, ImportScope::COMP_UNIT}})

  INS(void, Void);
  INS(boolean, Bool);
  INS(byte, Byte);
  INS(char, Char);
  INS(short, Short);
  INS(int, Int);

#undef INS

  u64 i = TypeId::kFirstRefTypeBase;
  for (const auto& type : types) {
    TypeBase next = i;
    i++;

    auto result = types_.insert(make_pair(type, next));
    // Verify that we didn't overwrite a key.
    assert(result.second);
  }

  for (const auto& bad_type : bad_types) {
    TypeBase next = TypeId::kErrorBase;

    auto result = types_.insert(make_pair(bad_type, next));
    // Verify that we didn't overwrite a key.
    assert(result.second);
  }
}

sptr<TypeSetImpl> TypeSetImpl::WithPackage(const string& package, base::ErrorList* errors) const {
  static const PosRange fakepos(-1, -1, -1);

  assert(pkg_prefix_ == "");

  string pkg = "";
  if (package == "") {
    pkg = kUnnamedPkgPrefix;
  } else {
    pkg = kNamedPkgPrefix + "." + package;
  }

  assert(pkgs_.count(pkg) == 1);

  TypeSetImpl view(*this);
  view.pkg_prefix_ = pkg + ".";
  view.InsertWildCard(ImportScope::PACKAGE, pkg, fakepos, errors);
  return sptr<TypeSetImpl>(new TypeSetImpl(view));
}

sptr<TypeSetImpl> TypeSetImpl::WithType(const string& name, base::PosRange pos, base::ErrorList* errors) const {
  TypeSetImpl view(*this);
  view.InsertAtScope(ImportScope::COMP_UNIT, pkg_prefix_ + name, pos, errors);
  return sptr<TypeSetImpl>(new TypeSetImpl(view));
}

sptr<TypeSetImpl> TypeSetImpl::WithImports(const vector<ast::ImportDecl>& imports, ErrorList* errors) const {
  static const string kJavaLangPrefix = kNamedPkgPrefix + ".java.lang";

  TypeSetImpl view(*this);

  if (pkgs_.count(kJavaLangPrefix) == 1) {
    PosRange fake(-1, -1, -1);
    view.InsertWildCard(ImportScope::WILDCARD, kJavaLangPrefix, fake, errors);
  }

  for (const auto& import : imports) {
    const string& fullname = kNamedPkgPrefix + "." + import.Name().Name();
    PosRange pos = import.Name().Tokens().front().pos;
    pos.end = import.Name().Tokens().back().pos.end;

    if (import.IsWildCard()) {
      view.InsertWildCard(ImportScope::WILDCARD, fullname, pos, errors);
    } else {
      view.InsertAtScope(ImportScope::COMP_UNIT, fullname, pos, errors);
    }
  }

  return make_shared<TypeSetImpl>(std::move(view));
}

void TypeSetImpl::InsertAtScope(ImportScope scope, const string& longname, PosRange pos, base::ErrorList* errors) {
  // Parse out the short version of the name.
  size_t last_dot = longname.find_last_of('.');
  assert(last_dot != string::npos);
  string shortname = longname.substr(last_dot + 1);

  // First, lookup this type.
  auto iter = types_.find(longname);
  if (iter == types_.end()) {
    errors->Append(MakeUnknownImportError(fs_, pos));

    // Blacklist both versions of this name.
    visible_types_.erase(longname);
    visible_types_.erase(shortname);

    TypeInfo blacklisted = TypeInfo{longname, TypeId::kErrorBase, ImportScope::COMP_UNIT};
    visible_types_.insert({longname, blacklisted});
    visible_types_.insert({shortname, blacklisted});
    return;
  }

  // If this type is already blacklisted, then do nothing.
  if (iter->second == TypeId::kErrorBase) {
    return;
  }


  TypeBase base = iter->second;

  // Lookup the shortname in the current visible set.
  TypeInfoMap::iterator begin;
  TypeInfoMap::iterator end;
  std::tie(begin, end) = visible_types_.equal_range(shortname);
  size_t num_entries = std::distance(begin, end);

  TypeInfo info = TypeInfo{longname, base, scope};

  // If there are no entries, or we are blacklisting a type, then just go ahead
  // and insert it.
  if (num_entries == 0 || base == TypeId::kErrorBase) {
    visible_types_.erase(begin, end);
    visible_types_.insert({shortname, info});
    return;
  }

  // If there are more than 1 entry in the map, then all of them must be
  // wildcard imports.
  if (num_entries > 1) {
    // If we are adding a non-wildcard import to a bunch of wildcard imports,
    // then the non-wildcard takes precedence, so erase all the other imports first.
    if (scope != ImportScope::WILDCARD) {
      visible_types_.erase(begin, end);
    }

    visible_types_.insert({shortname, info});
    return;
  }

  assert(num_entries == 1);
  TypeInfo prev = begin->second;

  // If this name was previously blacklisted, then just leave it blacklisted
  // and don't do anything.
  if (prev.base == TypeId::kErrorBase) {
    return;
  }

  // If the two types are the same, then we're done; we have to take the
  // highest scope of the two.
  if (info.base == prev.base) {
    begin->second.scope = std::min(info.scope, prev.scope);
    return;
  }

  // We know they are different types, now we consider their scopes. If one
  // scope is weaker than the other, the stronger one wins.
  if (info.scope != prev.scope) {
    if (info.scope < prev.scope) {
      begin->second = info;
      return;
    } else {
      // Leave the old value.
      return;
    }
  }

  // Now we know they're different types, in the same scope.  This is OK if
  // they are both wildcard imports.  If this is the case, we record the
  // conflict in our map, so that any reads of this entry will catch the error.
  if (info.scope == ImportScope::WILDCARD && prev.scope == ImportScope::WILDCARD) {
    visible_types_.insert({shortname, info});
    return;
  }

  // The only remaining case is that there are two identical names
  // compilation-unit level, which is an error. So we emit an error, and
  // blacklist this name.
  assert(info.base != prev.base);
  assert(info.scope == ImportScope::COMP_UNIT);
  assert(prev.scope == ImportScope::COMP_UNIT);

  // TODO: emit an error.
  throw;
}

TypeId TypeSetImpl::Get(const string& name, base::PosRange pos, base::ErrorList* errors) const {
  // For fully qualified names, look up in all types directly.
  if (name.find('.') != string::npos) {
    auto t = types_.find(kNamedPkgPrefix + '.' + name);
    if (t == types_.end()) {
      errors->Append(MakeUnknownTypenameError(fs_, pos));
      return TypeId::kUnassigned;
    }
    return TypeId{t->second, 0};
  }

  TypeInfoMap::const_iterator begin;
  TypeInfoMap::const_iterator end;
  std::tie(begin, end) = visible_types_.equal_range(name);
  size_t num_entries = std::distance(begin, end);

  if (num_entries == 0) {
    // TODO: return a special TypeId::kPackage if it is in pkgs_. That way the
    // TypeChecker can resolve ambiguous names correctly.
    errors->Append(MakeUnknownTypenameError(fs_, pos));
    return TypeId::kUnassigned;
  }

  // If there is an entry, then either it's an actual type, or it's
  // blacklisted.  In either case, just return the type.
  if (num_entries == 1) {
    return TypeId{begin->second.base, 0};
  }

  // There are multiple entries for this key, meaning that we have multiple
  // conflicting wildcard entries.
  assert(num_entries > 1);

  stringstream ss;
  ss << "'" << name << "' is ambiguous; it could refer to ";
  for (auto cur = begin; cur != end; ++cur) {
    if (cur != begin) {
      ss << ", or ";
    }
    ss << cur->second.full_name.substr(kPkgPrefixLen + 1);
  }
  ss << '.';
  errors->Append(MakeAmbiguousTypeError(fs_, pos, ss.str()));

  return TypeId::kError;
}

void TypeSetImpl::InsertWildCard(ImportScope scope, const string& basename, base::PosRange pos, base::ErrorList* errors) {
  if (pkgs_.count(basename) == 0) {
    errors->Append(MakeUnknownPackageError(fs_, pos));
    return;
  }

  auto prefix = basename + ".";
  auto begin = types_.lower_bound(prefix);
  auto end = types_.end();

  // Find all types that this wildcard import names.
  for (auto iter = begin; iter != end; ++iter) {
    auto decl = iter->first;

    // If the subsequent name is shorter, then we've passed the relevant
    // section of names starting with prefix.
    if (decl.size() < prefix.size()) {
      break;
    }

    // If we are importing "java.*", we don't want to make "java.lang.Integer"
    // available. So we only want types that don't have any dots in their name
    // after the package prefix.
    if (find(decl.begin() + prefix.size(), decl.end(), '.') != decl.end()) {
      continue;
    }

    // If the decl no longer starts with the prefix, then we've passed the
    // relevant section of names starting with prefix.
    if (decl.compare(0, prefix.size(), prefix) != 0) {
      break;
    }

    // Otherwise, we found a type that we should import, so we import it.
    InsertAtScope(scope, decl, pos, errors);
  }
}

} // namespace types
