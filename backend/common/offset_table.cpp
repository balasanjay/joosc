#include "backend/common/offset_table.h"

#include <algorithm>

#include "ast/ids.h"
#include "base/algorithm.h"
#include "base/printf.h"
#include "ir/size.h"

using std::tie;

using ast::FieldId;
using ast::MethodId;
using ast::TypeId;
using ast::TypeKind;
using base::FindEqualRanges;
using ir::SizeClass;
using ir::SizeClassFrom;
using types::FieldInfo;
using types::TypeInfo;
using types::TypeInfoMap;

namespace backend {

namespace {

// Return all non-inherited and non-static fields.
vector<FieldInfo> ExtractSimpleFields(const TypeInfo& tinfo) {
  vector<FieldInfo> fields;
  for (const auto& finfo_pair : tinfo.fields.GetFieldMap()) {
    const FieldInfo& finfo = finfo_pair.second;

    // Skip inherited fields.
    if (finfo.class_type != tinfo.type) {
      continue;
    }

    // Skip static fields.
    if (finfo.mods.HasModifier(lexer::STATIC)) {
      continue;
    }

    fields.emplace_back(finfo);
  }

  // Sort by field sizes in descending order.
  auto lt_cmp = [](const FieldInfo& lhs, const FieldInfo& rhs) {
    return SizeClassFrom(rhs.field_type) < SizeClassFrom(lhs.field_type);
  };
  std::stable_sort(fields.begin(), fields.end(), lt_cmp);

  return fields;
}

u64 ByteSizeFrom(SizeClass size, u8 ptr_size) {
  switch (size) {
    case SizeClass::BOOL: // Fall through.
    case SizeClass::BYTE:
      return 1;
    case SizeClass::SHORT: // Fall through.
    case SizeClass::CHAR:
      return 2;
    case SizeClass::INT:
      return 4;
    case SizeClass::PTR:
      return (u64)ptr_size;
    default:
      UNREACHABLE();
  }
}

u64 RoundUpToMultipleOf(u64 size, u64 multiple) {
  if ((size % multiple) == 0) {
    return size;
  }

  return size + (multiple - (size % multiple));
}

} // namespace

OffsetTable OffsetTable::Build(const TypeInfoMap& tinfo_map, u8 ptr_size) {
  map<FieldId, u64> field_offsets;
  map<TypeId, u64> type_sizes;

  vector<TypeInfo> types;
  for (const auto& type_pair : tinfo_map.GetTypeMap()) {
    const auto& tinfo = type_pair.second;
    // Ignore interfaces.
    if (tinfo.kind == TypeKind::CLASS) {
      types.emplace_back(tinfo);
    }
  }

  // Sort by the types' top-sort indices in ascending order.
  {
    auto lt_cmp = [&](const TypeInfo& lhs, const TypeInfo& rhs) {
      return lhs.top_sort_index < rhs.top_sort_index;
    };
    std::sort(types.begin(), types.end(), lt_cmp);
  }

  for (const auto& tinfo : types) {
    u64 parent_size = 0;
    if (tinfo.extends.Size() > 0) {
      CHECK(tinfo.extends.Size() == 1);

      parent_size = type_sizes.at(tinfo.extends.At(0));
    }

    u64 my_size = parent_size;

    for (const auto& finfo: ExtractSimpleFields(tinfo)) {
      u64 field_size = ByteSizeFrom(SizeClassFrom(finfo.field_type), ptr_size);
      auto iter = field_offsets.insert({finfo.fid, my_size});
      CHECK(iter.second);
      my_size += field_size;
    }

    my_size = RoundUpToMultipleOf(my_size, ptr_size);
    auto iter = type_sizes.insert({tinfo.type, my_size});
    CHECK(iter.second);
  }

  return OffsetTable(type_sizes, field_offsets, ptr_size);
}

} // namespace backend
