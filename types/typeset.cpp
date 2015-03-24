#include "types/typeset.h"

#include <algorithm>
#include <iostream>
#include <iterator>

#include "base/algorithm.h"
#include "types/types_internal.h"

using std::back_inserter;
using std::count;
using std::ostream;
using std::sort;
using std::transform;

using ast::CompUnit;
using ast::ImportDecl;
using ast::QualifiedName;
using ast::TypeId;
using base::DiagnosticClass;
using base::Error;
using base::ErrorList;
using base::FindEqualRanges;
using base::MakeError;
using base::OutputOptions;
using base::Pos;
using base::PosRange;

namespace types {

namespace {

Error* MakeDuplicateTypeDefinitionError(const string& name, const vector<PosRange>& defs) {
      stringstream msgstream;
      msgstream << "Type '" << name << "' was declared multiple times.";
      return MakeDuplicateDefinitionError(defs, msgstream.str(), "TypeDuplicateDefinitionError");
}

Error* MakeTypeWithTypePrefixError(PosRange pos, const string& name) {
  stringstream ss;
  ss << "'" << name << "' resolves as both a type and a package in this context.";
  return MakeSimplePosRangeError(pos, "TypeWithTypePrefixError", ss.str());
}

Error* MakeUnknownImportError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "UnknownImportError",
                                 "Cannot find imported class.");
}

Error* MakeUnknownPackageError(PosRange pos) {
  return MakeSimplePosRangeError(pos, "UnknownPackageError",
                                 "Cannot find imported package.");
}

Error* MakeAmbiguousTypeError(PosRange pos, const string& name, const vector<PosRange>& imports) {
  return MakeError([=](ostream* out, const OutputOptions& opt, const base::FileSet* fs) {
    if (opt.simple) {
      *out << "AmbiguousTypeError:[";
      for (auto pos : imports) {
        *out << pos << ",";
      }
      *out << "]";
      return;
    }

    string msg = "'" + name + "' is ambiguous.";
    PrintDiagnosticHeader(out, opt, fs, pos, DiagnosticClass::ERROR, msg);
    PrintRangePtr(out, opt, fs, pos);

    for (auto import : imports) {
      *out << '\n';

      // If fileid == -1, then we conflicted with the implicit import of
      // java.lang.*. Special-case this situation, and point the error back to
      // the standard usage.
      if (import.fileid == -1) {
        string msg = "Implicitly imported from java.lang.";
        PrintDiagnosticHeader(out, opt, fs, pos, DiagnosticClass::INFO, msg);
        continue;
      }

      string msg = "Imported here.";
      PrintDiagnosticHeader(out, opt, fs, import, DiagnosticClass::INFO, msg);
      PrintRangePtr(out, opt, fs, import);
    }
  });
}

} // namespace

void TypeSetBuilder::ExtractTypesAndPackages(
    vector<Type>* types,
    map<string, PosRange>* pkgs,
    map<int, string>* file_to_pkg) const {
  vector<sptr<const CompUnit>> units(units_);

  // Sort by FileId() in case CompUnits were added out of order.
  {
    using Unit = const sptr<const CompUnit>&;
    auto cmp = [](Unit lhs, Unit rhs) {
      return lhs->FileId() == rhs->FileId();
    };

    sort(units.begin(), units.end(), cmp);

    // Quickly verify that we don't have two compilation units with the same
    // FileId.
    auto iter = adjacent_find(units.begin(), units.end(), cmp);
    CHECK(iter == units.end());
  }

  // Bind references for readability.
  vector<Type>& all_types = *types;
  map<string, PosRange>& all_pkgs = *pkgs;

  TypeBase next = TypeId::kFirstRefTypeBase;

  for (const auto& spunit : units) {
    const CompUnit& unit = *spunit.get();

    // Compute the package name of this comp unit. While we're at it, declare
    // every package prefix in all_pkgs.
    string package = "";
    if (unit.PackagePtr() != nullptr) {
      const QualifiedName& name = *unit.PackagePtr();

      stringstream ss;
      ss << name.Parts().at(0);
      all_pkgs.insert({ss.str(), name.Tokens().at(0).pos});
      for (size_t i = 1; i < name.Parts().size(); ++i) {
        ss << '.' << name.Parts().at(i);

        PosRange pos = name.Tokens().at(0).pos;
        pos.end = name.Tokens().at(2*i).pos.end;

        all_pkgs.insert({ss.str(), pos});
      }

      package = ss.str();
    }

    file_to_pkg->insert({unit.FileId(), package});

    // Add all types, assigning TypeIds in order of declaration.
    for (const auto& decl : unit.Types()) {
      all_types.emplace_back(Type{
        decl.Name(),
        package + "." + decl.Name(),
        package,
        decl.NameToken().pos,
        next,
      });
      ++next;
    }
  }
}

void TypeSetBuilder::CheckTypePackageCollision(
    const map<string, PosRange>& all_pkgs, vector<Type>* types, ErrorList* errors) const {
  // Bind references for readability.
  vector<Type>& all_types = *types;

  for (Type& type : all_types) {
    auto iter = all_pkgs.find(type.longname);
    if (iter == all_pkgs.end()) {
      continue;
    }

    errors->Append(MakeDuplicateTypeDefinitionError(type.longname, {type.pos, iter->second}));
    type.tid = TypeId::kErrorBase;
  }
}

void TypeSetBuilder::BuildQualifiedNameIndex(
    vector<Type>* all_types_ptr,
    vector<Type>* qual_name_index,
    ErrorList* errors) const {
  // Bind references for readability.
  vector<Type>& all_types = *all_types_ptr;

  set<string> bad_names;
  vector<Type> types(all_types);

  // Sort.
  {
    auto cmp = [](const Type& lhs, const Type& rhs) {
      return lhs.longname < rhs.longname;
    };
    stable_sort(types.begin(), types.end(), cmp);
  }

  // Find duplicates.
  using Iter = vector<Type>::const_iterator;
  auto cmp = [](const Type& lhs, const Type& rhs) {
    return lhs.longname == rhs.longname;
  };
  auto cb = [&](Iter start, Iter end, i64 ndups) {
    if (ndups == 1) {
      return;
    }
    CHECK(ndups > 1);

    vector<PosRange> defs;
    for (auto cur = start; cur != end; ++cur) {
      defs.push_back(cur->pos);
    }
    CHECK(defs.size() == (size_t)ndups);

    string type = start->longname;
    // If this type is in the unnamed namespace, then strip off the leading
    // dot.
    if (start->pkg == "") {
      type = type.substr(1);
    }

    errors->Append(MakeDuplicateTypeDefinitionError(type, defs));
    bad_names.insert(start->longname);
  };

  FindEqualRanges(types.begin(), types.end(), cmp, cb);

  // Go through and blacklist any bad type that was identified.
  auto blacklist_vec = [&](vector<Type>* types) {
    for (Type& type : *types) {
      if (bad_names.count(type.longname) == 1) {
        type.tid = TypeId::kErrorBase;
      }
    }
  };
  blacklist_vec(&all_types);
  blacklist_vec(&types);

  // Copy all unique types to the qual_name_index.
  unique_copy(
      types.begin(),
      types.end(),
      back_inserter(*qual_name_index),
      cmp);

  // Remove all duplicates from all_types.
  all_types.erase(unique(all_types.begin(), all_types.end(), cmp), all_types.end());
}

void TypeSetBuilder::ResolveImports(
    const map<string, PosRange>& all_pkgs,
    const vector<Type>& qual_name_index,
    vector<Type>* comp_unit_scope,
    vector<WildCardImport>* wildcards,
    base::ErrorList* errors) const {
  auto lookup_by_qual_name_cmp = [](const Type& lhs, const string& rhs) {
    return lhs.longname < rhs;
  };
  auto must_resolve = [&](const string& qual_name, PosRange pos) {
    auto iter = lower_bound(
        qual_name_index.begin(),
        qual_name_index.end(),
        qual_name,
        lookup_by_qual_name_cmp);

    if (iter == qual_name_index.end() || iter->longname != qual_name) {
      errors->Append(MakeUnknownImportError(pos));
      return qual_name_index.end();
    }

    return iter;
  };

  using WC = WildCardImport;
  auto wildcard_cmp = [](const WC& lhs, const WC& rhs) {
    return tie(lhs.fid, lhs.pkg) < tie(rhs.fid, rhs.pkg);
  };
  set<WC, decltype(wildcard_cmp)> wcs(wildcard_cmp);

  for (const auto& spunit : units_) {
    const CompUnit& unit = *spunit.get();

    wcs.insert({WildCardImport{unit.FileId(), "java.lang", PosRange(-1, -1, -1)}});

    vector<Type> file_types;

    // First, add all imports.
    for (const auto& import : unit.Imports()) {
      PosRange pos = import.Name().Tokens().front().pos;
      pos.end = import.Name().Tokens().back().pos.end;

      if (import.IsWildCard()) {
        if (all_pkgs.count(import.Name().Name()) == 1) {
          wcs.insert(WildCardImport{unit.FileId(), import.Name().Name(), pos});
        } else {
          errors->Append(MakeUnknownPackageError(pos));
        }
        continue;
      }

      auto iter = must_resolve(import.Name().Name(), pos);

      TypeBase tid = TypeId::kErrorBase;
      if (iter != qual_name_index.end()) {
        tid = iter->tid;
      }

      file_types.emplace_back(Type{
        import.Name().Parts().back(),
        "",
        "",
        pos,
        tid,
      });
    }

    // Second, add all types.
    {
      string package = "";
      if (unit.PackagePtr() != nullptr) {
        package = unit.PackagePtr()->Name();
      }

      for (const auto& decl : unit.Types()) {
        auto iter = must_resolve(package + "." + decl.Name(), decl.NameToken().pos);
        CHECK(iter != qual_name_index.end());

        file_types.emplace_back(Type{
          iter->simple_name,
          "",
          "",
          decl.NameToken().pos,
          iter->tid,
        });
      }
    }

    // Third, drop any duplicate imports.
    {
      auto lt_cmp = [](const Type& lhs, const Type& rhs) {
        if (lhs.simple_name != rhs.simple_name) {
          return lhs.simple_name < rhs.simple_name;
        }

        return lhs.tid < rhs.tid;
      };
      auto eq_cmp = [&](const Type& lhs, const Type& rhs) {
        return !lt_cmp(lhs, rhs) && !lt_cmp(rhs, lhs);
      };
      stable_sort(file_types.begin(), file_types.end(), lt_cmp);

      auto new_end = unique(file_types.begin(), file_types.end(), eq_cmp);
      file_types.erase(new_end, file_types.end());
    }

    // Fourth, check for conflicting entries.
    {
      using Iter = vector<Type>::const_iterator;
      auto cmp = [](const Type& lhs, const Type& rhs) {
        return lhs.simple_name == rhs.simple_name;
      };
      auto cb = [&](Iter start, Iter end, i64 ndups) {
        if (ndups == 1) {
          comp_unit_scope->emplace_back(*start);
          return;
        }
        CHECK(ndups > 1);

        vector<PosRange> defs;
        for (auto cur = start; cur != end; ++cur) {
          defs.push_back(cur->pos);
        }
        CHECK(defs.size() == (size_t)ndups);

        string type = start->simple_name;
        errors->Append(MakeDuplicateTypeDefinitionError(type, defs));

        // Blacklist this name.
        comp_unit_scope->emplace_back(Type{
          type,
          "",
          "",
          start->pos,
          TypeId::kErrorBase,
        });
      };
      FindEqualRanges(file_types.begin(), file_types.end(), cmp, cb);
    }
  }
  wildcards->insert(wildcards->end(), wcs.begin(), wcs.end());
}

auto TypeSetBuilder::ResolvePkgScope(const vector<Type>& all_types) const -> vector<Type> {
  vector<Type> types(all_types);

  auto lt_cmp = [](const Type& lhs, const Type& rhs) {
    return tie(lhs.pkg, lhs.simple_name) < tie(rhs.pkg, rhs.simple_name);
  };

  sort(types.begin(), types.end(), lt_cmp);

  return types;
}

TypeSet TypeSetBuilder::Build(ErrorList* errors) const {
  vector<Type> all_types;
  map<string, PosRange> all_pkgs;
  map<int, string> file_pkg_index;

  ExtractTypesAndPackages(&all_types, &all_pkgs, &file_pkg_index);

  // Check for a type having the same name as a package.
  CheckTypePackageCollision(all_pkgs, &all_types, errors);

  // Build an index by qualified name. Will also check for multiple types with
  // the same qualified name.
  vector<Type> qual_name_index;
  BuildQualifiedNameIndex(&all_types, &qual_name_index, errors);

  // Resolve comp-unit-scoped types. Specifically, direct type declarations,
  // and single-import declarations.
  vector<Type> comp_unit_scope;
  vector<WildCardImport> wildcards;
  ResolveImports(all_pkgs, qual_name_index, &comp_unit_scope, &wildcards, errors);

  // Resolve package scoped types.
  vector<Type> pkg_scope = ResolvePkgScope(all_types);

  Data* data = new Data{
    comp_unit_scope,
    pkg_scope,
    wildcards,
    {}, // wildcard_lookup_cache_ starts off empty.
    qual_name_index,
    file_pkg_index,
  };

  return TypeSet(sptr<Data>(data));
}

auto TypeSet::LookupInPkgScope(const string& pkg, const string& name) const -> const Type* {
  auto cmp = [](const Type& lhs, const pair<string, string>& rhs) {
    return tie(lhs.pkg, lhs.simple_name) < tie(rhs.first, rhs.second);
  };
  auto lookup_val = make_pair(pkg, name);

  auto iter = lower_bound(
      data_->pkg_scope_.begin(),
      data_->pkg_scope_.end(),
      lookup_val,
      cmp);
  if (iter != data_->pkg_scope_.end() &&
      iter->pkg == pkg &&
      iter->simple_name == name) {
    return &(*iter);
  }
  return nullptr;
}

TypeId TypeSet::Get(const string& name, PosRange pos, ErrorList* errors) const {
  // First, handle any of the built-ins.
  {
    #define BUILTIN_RETURN(Type, type) { \
      if (name == #type) { return TypeId{TypeId::k##Type##Base, 0}; } \
    }
    BUILTIN_RETURN(Void, void);
    BUILTIN_RETURN(Bool, boolean);
    BUILTIN_RETURN(Byte, byte);
    BUILTIN_RETURN(Char, char);
    BUILTIN_RETURN(Short, short);
    BUILTIN_RETURN(Int, int);
    #undef BUILTIN_RETURN
  }

  // Second, we handle fully qualified names.
  {
    auto first_dot = name.find('.');
    if (first_dot != string::npos) {
      auto cmp = [](const Type& lhs, const string& rhs) {
        return lhs.longname < rhs;
      };
      auto iter = lower_bound(
          data_->qual_name_index_.begin(),
          data_->qual_name_index_.end(),
          name,
          cmp);

      // If we couldn't find a matching fully-qualified-name, then emit an error,
      // and return.
      if (iter == data_->qual_name_index_.end() || iter->longname != name) {
        errors->Append(MakeUnknownTypenameError(pos));
        return TypeId::kUnassigned;
      }

      // If this type has been blacklisted, then just return it immediately.
      if (iter->tid == TypeId::kErrorBase) {
        return TypeId::kError;
      }

      // Check that the first element of the qualified name does not also
      // resolve to a type in the current environment.
      //
      // Technically, every other prefix of the qualified name also should not
      // resolve to a type. However, these other prefixes cannot resolve to a
      // type. They are a package in this context, and our earlier check that
      // we don't have a package and a class named the same thing would have
      // caught this case.
      string first_seg = name.substr(0, first_dot);
      TypeId tid = TryGet(first_seg);
      if (tid.IsValid()) {
        errors->Append(MakeTypeWithTypePrefixError(pos, first_seg));
        return TypeId::kError;
      }

      return TypeId{iter->tid, 0};
    }
  }

  // All further searches depend on being inside a compilation unit. So bail
  // early if we aren't in one.
  if (fid_ == -1) {
    errors->Append(MakeUnknownTypenameError(pos));
    return TypeId::kUnassigned;
  }

  // Third, try finding this type in comp-unit scope.
  {
    auto cmp = [](const Type& lhs, const pair<int, string>& rhs) {
      return tie(lhs.pos.fileid, lhs.simple_name) < tie(rhs.first, rhs.second);
    };
    auto lookup_val = make_pair(fid_, name);

    auto iter = lower_bound(
        data_->comp_unit_scope_.begin(),
        data_->comp_unit_scope_.end(),
        lookup_val,
        cmp);
    if (iter != data_->comp_unit_scope_.end() &&
        iter->pos.fileid == fid_ &&
        iter->simple_name == name) {
      return TypeId{iter->tid, 0};
    }
  }

  // Fourth, try finding this type in package scope.
  {
    const Type* type = LookupInPkgScope(pkg_, name);
    if (type != nullptr) {
      return TypeId{type->tid, 0};
    }
  }

  // Fifth, check the wildcard cache.
  {
    auto iter = data_->wildcard_lookup_cache_.find(make_pair(fid_, name));
    if (iter != data_->wildcard_lookup_cache_.end()) {
      return TypeId{iter->second, 0};
    }
  }

  // Finally, do the full wildcard lookup, and cache the result.
  using WC = WildCardImport;
  auto lookup_val = WC{fid_, "", PosRange(-1, -1, -1)};
  auto cmp = [](const WC& lhs, const WC& rhs) {
    return lhs.fid < rhs.fid;
  };

  auto iter_pair = equal_range(
    data_->wildcard_imports_.begin(),
    data_->wildcard_imports_.end(),
    lookup_val,
    cmp);

  // Try each wildcard-import to see if we can find a matching type.
  vector<Type> matches;
  for (auto cur = iter_pair.first; cur != iter_pair.second; ++cur) {
    const Type* type = LookupInPkgScope(cur->pkg, name);
    if (type != nullptr) {
      // Copy the type, and overwrite the pos field to indicate where it was
      // imported from, rather than where it was declared. This is used when
      // generating the ambiguous wildcard import error below.
      Type t = *type;
      t.pos = cur->pos;
      matches.emplace_back(t);
    }
  }

  auto cache_key = make_pair(fid_, name);
  auto cache_val = TypeId::kErrorBase;
  if (matches.size() == 0) {
    errors->Append(MakeUnknownTypenameError(pos));
  } else if (matches.size() == 1) {
    cache_val = matches[0].tid;
  } else {
    CHECK(matches.size() > 1);
    vector<PosRange> imports;
    for (const auto& match : matches) {
      imports.push_back(match.pos);
    }
    errors->Append(MakeAmbiguousTypeError(pos, name, imports));
  }
  data_->wildcard_lookup_cache_.insert({cache_key, cache_val});
  return TypeId{cache_val, 0};
}

} // namespace types
