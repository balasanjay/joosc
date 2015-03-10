#include "types/typeset_impl.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include "types/types_internal.h"

using std::back_inserter;
using std::count;
using std::sort;
using std::transform;

using ast::ImportDecl;
using ast::TypeId;
using base::DiagnosticClass;
using base::Error;
using base::ErrorList;
using base::MakeError;
using base::OutputOptions;
using base::Pos;
using base::PosRange;

namespace types {

namespace {

Error* MakeUnknownImportError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "UnknownImportError",
                                 "Cannot find imported class.");
}

Error* MakeAmbiguousTypeError(PosRange pos, const string& msg) {
  return MakeSimplePosRangeError(pos, "AmbiguousType", msg);
}

Error* MakeUnknownPackageError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "UnknownPackageError",
                                 "Cannot find imported package.");
}

Error* MakePackageTypeAmbiguityError(PosRange first, PosRange second) {
  // TODO: use MakeError.
  return MakeSimplePosRangeError(first, "PackageTypeAmbiguityError",
                                 "PackageTypeAmbiguity");
}

} // namespace

const int TypeSetImpl::kPkgPrefixLen = 3;
const string TypeSetImpl::kUnnamedPkgPrefix = "<0>";
const string TypeSetImpl::kNamedPkgPrefix = "<1>";

TypeSetImpl::TypeSetImpl(const map<string, PosRange>& types, const map<string, PosRange>& pkgs, const set<string>& bad_types) : pkgs_(pkgs) {
  // Insert all predefined types.
#define INS(name, Name) { \
  TypeBase _base = TypeId::k##Name##Base; \
  types_.insert({#name, make_pair(_base, kFakePos)}); \
  visible_types_.insert(TypeInfo{#name, TypeId::k##Name##Base, ImportScope::COMP_UNIT, #name, kFakePos}); \
}

  INS(void, Void);
  INS(boolean, Bool);
  INS(byte, Byte);
  INS(char, Char);
  INS(short, Short);
  INS(int, Int);

#undef INS

  u64 i = TypeId::kFirstRefTypeBase;
  for (const auto& type : types) {
    string type_name = type.first;
    PosRange type_pos = type.second;

    TypeBase next = i;
    i++;

    auto result = types_.insert({type_name, {next, type_pos}});
    // Verify that we didn't overwrite a key.
    CHECK(result.second);
  }

  for (const auto& bad_type : bad_types) {
    TypeBase error = TypeId::kErrorBase;
    const string& type_name = bad_type;
    auto result = types_.insert({type_name, {error, kFakePos}});
    // Verify that we didn't overwrite a key.
    CHECK(result.second);
  }
}

sptr<TypeSetImpl> TypeSetImpl::WithPackage(const string& package, PosRange pos, base::ErrorList* errors) const {
  CHECK(pkg_prefix_ == "");

  string pkg = "";
  if (package == "") {
    pkg = kUnnamedPkgPrefix;
  } else {
    pkg = kNamedPkgPrefix + "." + package;
  }

  CHECK(pkgs_.count(pkg) == 1);

  TypeSetImpl view(*this);
  view.pkg_prefix_ = pkg + ".";
  view.InsertWildCard(ImportScope::PACKAGE, pkg, pos, errors);
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
    view.InsertWildCard(ImportScope::WILDCARD, kJavaLangPrefix, kFakePos, errors);
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

auto TypeSetImpl::FindByShortName(const string& shortname) const -> pair<VisibleSet::const_iterator, VisibleSet::const_iterator> {
  TypeInfo query = {shortname, 0, ImportScope::COMP_UNIT, "", kFakePos};
  auto iter = visible_types_.lower_bound(query);
  if (iter == visible_types_.end()) {
    return make_pair(iter, iter);
  }

  auto start = iter;
  auto end = iter;
  for (; end != visible_types_.end(); ++end) {
    if (end->shortname != shortname) {
      break;
    }
  }

  return make_pair(start, end);
}

void TypeSetImpl::InsertAtScope(ImportScope scope, const string& longname, PosRange pos, base::ErrorList* errors) {
  // Parse out the short version of the name.
  size_t last_dot = longname.find_last_of('.');
  CHECK(last_dot != string::npos);
  string shortname = longname.substr(last_dot + 1);

  // First, lookup this type.
  auto iter = types_.find(longname);
  if (iter == types_.end()) {
    errors->Append(MakeUnknownImportError(pos));

    // Erase both versions of the name.
    auto long_iter = FindByShortName(longname);
    auto short_iter = FindByShortName(shortname);
    visible_types_.erase(long_iter.first, long_iter.second);
    visible_types_.erase(short_iter.first, short_iter.second);

    // Blacklist both versions of the name.
    visible_types_.insert(TypeInfo{longname, TypeId::kErrorBase, ImportScope::COMP_UNIT, longname, kFakePos});
    visible_types_.insert(TypeInfo{shortname, TypeId::kErrorBase, ImportScope::COMP_UNIT, longname, kFakePos});
    return;
  }

  TypeBase base = iter->second.first;
  PosRange base_pos = iter->second.second;

  // Lookup the shortname in the current visible set.
  VisibleSet::iterator begin;
  VisibleSet::iterator end;
  std::tie(begin, end) = FindByShortName(shortname);
  size_t num_entries = std::distance(begin, end);

  TypeInfo info = TypeInfo{shortname, base, scope, longname, pos};

  // If this type has already been blacklisted, then overwrite any entries with
  // a blacklist entry.
  if (base == TypeId::kErrorBase) {
    visible_types_.erase(begin, end);
    visible_types_.insert(info);
    return;
  }

  // If we import a type a.b.c, and there is another package c.d, then this
  // would introduce an ambiguity when we see "c.d". The only exception to this
  // rule is a wildcard import. In this case, we allow the import, but fail
  // when the import is used.
  auto types_iter = types_.find(kNamedPkgPrefix + "." + shortname);
  if (scope != ImportScope::WILDCARD && types_iter != types_.end()) {
    PosRange pkg_pos = PosRange(0, 0, 0);
    errors->Append(MakePackageTypeAmbiguityError(pkg_pos, pkg_pos));

    // TODO: consider blacklisting shortname.
    return;
  }

  // If there are no entries, then just go ahead and insert it.
  if (num_entries == 0) {
    visible_types_.insert(info);
    return;
  }

  // If there is more than 1 entry in the set, then all of them must be
  // wildcard imports of unique types.
  if (num_entries > 1) {
    // If we are adding a non-wildcard import to a bunch of wildcard imports,
    // then the non-wildcard takes precedence, so erase all the other imports first.
    if (scope != ImportScope::WILDCARD) {
      visible_types_.erase(begin, end);
    }

    visible_types_.insert(info);
    return;
  }

  CHECK(num_entries == 1);
  TypeInfo prev = *begin;

  // If this name was previously blacklisted, then just leave it blacklisted
  // and don't do anything.
  if (prev.base == TypeId::kErrorBase) {
    return;
  }

  // If the two types are the same, then we're done; we have to take the
  // highest scope of the two.
  if (info.base == prev.base) {
    info.scope = std::min(info.scope, prev.scope);
    visible_types_.erase(begin);
    visible_types_.insert(info);
    return;
  }

  // We know they are different types, now we consider their scopes. If one
  // scope is weaker than the other, the stronger one wins.
  if (info.scope != prev.scope) {
    if (info.scope < prev.scope) {
      visible_types_.erase(begin);
      visible_types_.insert(info);
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
    visible_types_.insert(info);
    return;
  }

  // The only remaining case is that there are two identical names
  // compilation-unit level, which is an error. So we emit an error, and
  // blacklist this name.
  CHECK(info.base != prev.base);
  CHECK(info.scope == ImportScope::COMP_UNIT);
  CHECK(prev.scope == ImportScope::COMP_UNIT);

  // TODO: emit an error.
  errors->Append(MakeUnknownTypenameError(pos));

  // TODO: blacklist.
}

TypeId TypeSetImpl::Get(const string& name, base::PosRange pos, base::ErrorList* errors) const {
  // For fully qualified names, look up in all types directly.
  if (name.find('.') != string::npos) {
    string qualified = kNamedPkgPrefix + '.' + name;
    auto t = types_.find(qualified);
    if (t == types_.end()) {
      errors->Append(MakeUnknownTypenameError(pos));
      return TypeId::kUnassigned;
    }

    size_t search_begin = 0;
    while (true) {
      size_t next_dot = name.find('.', search_begin);
      if (next_dot == string::npos) {
        break;
      }

      string candidate = name.substr(0, next_dot);
      search_begin = next_dot + 1;

      // Look up candidate as a fully-qualified globally unique name.
      if (types_.count(kNamedPkgPrefix + "." + candidate)) {
        // TODO: errors.
        PosRange pos(0, 0, 0);
        errors->Append(MakeUnknownTypenameError(pos));
        break;
      }

      // Look up candidate in the current environment.
      if (candidate.find('.') == string::npos) {
        auto iter_pair = FindByShortName(candidate);
        if (iter_pair.first != iter_pair.second) {
          PosRange pos(0, 0, 0);
          errors->Append(MakeUnknownTypenameError(pos));
          break;
        }
      }
    }

    return TypeId{t->second.first, 0};
  }

  VisibleSet::const_iterator begin;
  VisibleSet::const_iterator end;
  std::tie(begin, end) = FindByShortName(name);
  size_t num_entries = std::distance(begin, end);

  if (num_entries == 0) {
    errors->Append(MakeUnknownTypenameError(pos));
    return TypeId::kUnassigned;
  }

  // If there is an entry, then either it's an actual type, or it's
  // blacklisted.  In either case, just return the type.
  if (num_entries == 1) {
    return TypeId{begin->base, 0};
  }

  // There are multiple entries for this key, meaning that we have multiple
  // conflicting wildcard entries.
  CHECK(num_entries > 1);

  stringstream ss;
  ss << "'" << name << "' is ambiguous; it could refer to ";
  for (auto cur = begin; cur != end; ++cur) {
    if (cur != begin) {
      ss << ", or ";
    }
    ss << cur->longname.substr(kPkgPrefixLen + 1);
  }
  ss << '.';
  errors->Append(MakeAmbiguousTypeError(pos, ss.str()));

  return TypeId::kError;
}

void TypeSetImpl::InsertWildCard(ImportScope scope, const string& basename, base::PosRange pos, base::ErrorList* errors) {
  if (pkgs_.count(basename) == 0) {
    errors->Append(MakeUnknownPackageError(pos));
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
