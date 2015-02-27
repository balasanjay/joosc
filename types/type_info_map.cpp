#include "types/type_info_map.h"

#include "base/algorithm.h"
#include "types/types_internal.h"

using base::FindEqualRanges;
using base::PosRange;

namespace types {

TypeInfoMap TypeInfoMap::kEmptyTypeInfoMap = TypeInfoMap();
MethodTable MethodTable::kEmptyMethodTable = MethodTable();

void TypeInfoMapBuilder::CheckMethodDuplicates(const base::FileSet* fs, base::ErrorList* out) {
  vector<MethodInfo> entries(method_entries_);
  stable_sort(entries.begin(), entries.end());

  using Iter = vector<MethodInfo>::const_iterator;

  auto cmp = [](const MethodInfo& lhs, const MethodInfo& rhs) {
    return lhs.class_type == rhs.class_type &&
           lhs.signature == rhs.signature;
  };

  auto cb = [&](Iter start, Iter end, i64 ndups) {
    if (ndups == 1) {
      // TODO: insert into useful list.
      return;
    }

    assert(ndups > 1);

    // TODO: insert into method blacklist.

    vector<PosRange> defs;
    for (auto cur = start; cur != end; ++cur) {
      defs.push_back(cur->namepos);
    }
    stringstream msgstream;
    msgstream << "Method '" << start->signature.name << "' was declared multiple times.";
    out->Append(MakeDuplicateDefinitionError(fs, defs, msgstream.str(), start->signature.name));
  };

  FindEqualRanges(entries.cbegin(), entries.cend(), cmp, cb);
}

} // namespace types
