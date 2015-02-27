#include "types/type_info_map.h"

#include "base/algorithm.h"
#include "types/types_internal.h"

using base::Error;
using base::FileSet;
using base::FindEqualRanges;
using base::PosRange;

namespace types {

TypeInfoMap TypeInfoMap::kEmptyTypeInfoMap = TypeInfoMap(vector<TypeInfo>{});
MethodTable MethodTable::kEmptyMethodTable = MethodTable(vector<MethodTableParam>{});

Error* TypeInfoMapBuilder::MakeConstructorNameError(const FileSet* fs, PosRange pos) const {
  return MakeSimplePosRangeError(fs, pos, "ConstructorNameError", "Constructors must have the same name as its class.");
}

TypeInfoMap TypeInfoMapBuilder::Build(const base::FileSet* fs, base::ErrorList* out) {
  vector<MethodInfo> good_methods;
  vector<MethodInfo> bad_methods;
  {
    vector<MethodInfo> entries(method_entries_);
    stable_sort(entries.begin(), entries.end());

    using Iter = vector<MethodInfo>::const_iterator;

    auto cmp = [](const MethodInfo& lhs, const MethodInfo& rhs) {
      return lhs.class_type == rhs.class_type &&
             lhs.signature == rhs.signature;
    };

    auto cb = [&](Iter start, Iter end, i64 ndups) {
      if (ndups == 1) {
        good_methods.push_back(*start);
        return;
      }

      assert(ndups > 1);

      vector<PosRange> defs;
      for (auto cur = start; cur != end; ++cur) {
        bad_methods.push_back(*cur);
        defs.push_back(cur->namepos);
      }
      stringstream msgstream;
      msgstream << "Method '" << start->signature.name << "' was declared multiple times.";
      out->Append(MakeDuplicateDefinitionError(fs, defs, msgstream.str(), start->signature.name));
    };

    FindEqualRanges(entries.cbegin(), entries.cend(), cmp, cb);
  }

  sort(type_entries_.begin(), type_entries_.end());
  auto good_iter = good_methods.begin();
  auto bad_iter = bad_methods.begin();
  MethodId cur_mid = 0;
  vector<TypeInfo> type_info_entries;

  // TODO: Do something useful with method blacklist.

  for (const auto& type_entry : type_entries_) {
    const string& type_name = type_entry.name;
    vector<MethodTableParam> method_table_entries;
    for (; bad_iter != bad_methods.end(); ++bad_iter) {
      if (bad_iter->class_type != type_entry.type) {
        break;
      }
      if (bad_iter->is_constructor && bad_iter->signature.name != type_name) {
        out->Append(MakeConstructorNameError(fs, bad_iter->namepos));
      }
    }
    for (; good_iter != good_methods.end(); ++good_iter) {
      if (good_iter->class_type != type_entry.type) {
        break;
      }
      if (good_iter->is_constructor && good_iter->signature.name != type_name) {
        out->Append(MakeConstructorNameError(fs, good_iter->namepos));
        // TODO: add to method blacklist?
        continue;
      }

      method_table_entries.push_back(MethodTableParam{*good_iter, cur_mid++});
    }
    type_info_entries.push_back(TypeInfo{type_entry.mods, type_entry.kind, type_entry.type, type_entry.name, type_entry.pos, type_entry.extends, type_entry.implements, MethodTable(method_table_entries)});
  }

  return TypeInfoMap(type_info_entries);
}

} // namespace types
