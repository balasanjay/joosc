#include "types/type_info_map.h"

#include "base/algorithm.h"
#include "types/types_internal.h"

using std::tie;

using ast::TypeId;
using base::Error;
using base::ErrorList;
using base::FileSet;
using base::FindEqualRanges;
using base::PosRange;

namespace types {

TypeInfoMap TypeInfoMap::kEmptyTypeInfoMap = TypeInfoMap({});
MethodTable MethodTable::kEmptyMethodTable = MethodTable({}, {}, false);
MethodTable MethodTable::kErrorMethodTable = MethodTable();

Error* TypeInfoMapBuilder::MakeConstructorNameError(PosRange pos) const {
  return MakeSimplePosRangeError(fs_, pos, "ConstructorNameError", "Constructors must have the same name as its class.");
}

// Builds valid MethodTables for a TypeInfo. Emits errors if methods for the
// type are invalid.
void TypeInfoMapBuilder::BuildMethodTable(MInfoIter begin, MInfoIter end, TypeInfo* tinfo, MethodId* cur_mid, const map<TypeId, TypeInfo>& sofar, ErrorList* out) {
  // Sort all MethodInfo to cluster them by signature.
  auto lt_cmp = [](const MethodInfo& lhs, const MethodInfo& rhs) {
    return tie(lhs.is_constructor, lhs.signature) < tie(rhs.is_constructor, rhs.signature);
  };
  stable_sort(begin, end, lt_cmp);

  vector<MethodTableParam> good_methods;
  set<string> bad_methods;
  bool has_bad_constructor = false;

  // Build MethodTable ignoring parent methods.
  {
    auto eq_cmp = [&lt_cmp](const MethodInfo& lhs, const MethodInfo& rhs) {
      return !lt_cmp(lhs, rhs) && !lt_cmp(rhs, lhs);
    };

    auto cb = [&](MInfoCIter lbegin, MInfoCIter lend, i64 ndups) {
      // Make sure constructors are named the same as the class.
      for (auto cur = lbegin; cur != lend; ++cur) {
        if (cur->is_constructor && cur->signature.name != tinfo->name) {
          out->Append(MakeConstructorNameError(cur->pos));
          has_bad_constructor = true;
        }
      }

      // Add non-duplicate MethodInfo to the MethodTable.
      if (ndups == 1) {
        good_methods.push_back({*lbegin, *cur_mid});
        ++(*cur_mid);
        return;
      }

      // Emit error for duped methods.
      assert(ndups > 1);

      vector<PosRange> defs;
      for (auto cur = lbegin; cur != lend; ++cur) {
        defs.push_back(cur->pos);
      }
      stringstream msgstream;
      if (lbegin->is_constructor) {
        msgstream << "Constructor";
      } else {
        msgstream << "Method";
      }
      msgstream << " '" << lbegin->signature.name << "' was declared multiple times.";
      out->Append(MakeDuplicateDefinitionError(fs_, defs, msgstream.str(), lbegin->signature.name));
      bad_methods.insert(lbegin->signature.name);
    };

    FindEqualRanges(begin, end, eq_cmp, cb);
  }

  // TODO: update MethodTable with methods from parents.
  tinfo->methods = MethodTable(good_methods, bad_methods, has_bad_constructor);
}

TypeInfoMap TypeInfoMapBuilder::Build(base::ErrorList* out) {
  map<TypeId, TypeInfo> typeinfo;
  for (const auto& entry : type_entries_) {
    typeinfo.insert({entry.type, entry});
  }

  // TODO(sjy): assign top_sort_index.

  // Sort MethodInfo vector by the topological ordering of the types.
  {
    auto cmp = [&typeinfo](const MethodInfo& lhs, const MethodInfo& rhs) {
      return typeinfo.at(lhs.class_type).top_sort_index < typeinfo.at(rhs.class_type).top_sort_index;
    };
    stable_sort(method_entries_.begin(), method_entries_.end(), cmp);
  }

  // Populate MethodTables for each TypeInfo.
  {
    MethodId cur_mid = 1;

    auto cmp = [](const MethodInfo& lhs, const MethodInfo& rhs) {
      return lhs.class_type == rhs.class_type;
    };

    auto cb = [&](MInfoIter begin, MInfoIter end, i64) {
      TypeInfo* tinfo = &typeinfo.at(begin->class_type);
      BuildMethodTable(begin, end, tinfo, &cur_mid, typeinfo, out);
    };

    FindEqualRanges(method_entries_.begin(), method_entries_.end(), cmp, cb);
  }

  return TypeInfoMap(typeinfo);
}

} // namespace types
