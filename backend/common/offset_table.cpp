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
using ast::kUnassignedMethodId;
using base::FindEqualRanges;
using ir::ByteSizeFrom;
using ir::SizeClass;
using ir::SizeClassFrom;
using types::FieldInfo;
using types::MethodInfo;
using types::TypeInfo;
using types::TypeInfoMap;

namespace backend {
namespace common {

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

u64 RoundUpToMultipleOf(u64 size, u64 multiple) {
  if ((size % multiple) == 0) {
    return size;
  }

  return size + (multiple - (size % multiple));
}

void BuildTypeSizesAndFieldOffsets(const vector<TypeInfo>& types, u8 ptr_size,
    OffsetTable::TypeMap* type_out, OffsetTable::FieldMap* field_out) {
  for (const auto& tinfo : types) {
    // Skip interfaces since they don't have fields.
    if (tinfo.kind == TypeKind::INTERFACE) {
      continue;
    }

    u64 parent_size = 0;
    if (tinfo.extends.Size() > 0) {
      CHECK(tinfo.extends.Size() == 1);
      parent_size = type_out->at(tinfo.extends.At(0));
    }

    u64 my_size = parent_size;

    for (const auto& finfo: ExtractSimpleFields(tinfo)) {
      u64 field_size = ByteSizeFrom(SizeClassFrom(finfo.field_type), ptr_size);
      auto iter = field_out->insert({finfo.fid, my_size});
      CHECK(iter.second);
      my_size += field_size;
    }

    my_size = RoundUpToMultipleOf(my_size, (u64)ptr_size);
    auto iter = type_out->insert({tinfo.type, my_size});
    CHECK(iter.second);
  }
}

void BuildMethodOffsets(const vector<TypeInfo>& types, u8 ptr_size,
    OffsetTable::MethodMap* method_out, OffsetTable::VtableMap* vtable_out) {
  map<ast::TypeId, u64> starting_offsets;

  using Vpair = pair<TypeId, MethodId>;
  auto vtable_cmp = [&](const Vpair& lhs, const Vpair& rhs) {
    return method_out->at(lhs.second) < method_out->at(rhs.second);
  };

  for (const TypeInfo& tinfo : types) {
    // Interface methods will use a different lookup.
    if (tinfo.kind == TypeKind::INTERFACE) {
      continue;
    }

    u64 starting_offset = 0;
    if (tinfo.extends.Size() > 0) {
      CHECK(tinfo.extends.Size() == 1);
      starting_offset = starting_offsets.at(tinfo.extends.At(0));
    }

    u64 my_offset = starting_offset;

    OffsetTable::Vtable vtable;

    for (const auto& minfo_pair : tinfo.methods.GetMethodMap()) {
      const MethodInfo& minfo = minfo_pair.second;

      if (minfo.mods.HasModifier(lexer::STATIC)) {
        continue;
      }

      if (minfo.signature.is_constructor) {
        continue;
      }

      // Skip inherited methods.
      if (minfo.class_type != tinfo.type) {
        vtable.push_back({minfo.class_type, minfo.mid});
        continue;
      }

      vtable.push_back({tinfo.type, minfo.mid});

      if (minfo.parent_mid != kUnassignedMethodId) {
        // Use the parent offset to enable overriding.
        method_out->insert({minfo.mid, method_out->at(minfo.parent_mid)});
        continue;
      }

      method_out->insert({minfo.mid, my_offset});
      my_offset += (u64)ptr_size;
    }

    {
      auto iter = starting_offsets.insert({tinfo.type, my_offset});
      CHECK(iter.second);
    }

    {
      std::sort(vtable.begin(), vtable.end(), vtable_cmp);
      auto iter = vtable_out->insert({tinfo.type, vtable});
      CHECK(iter.second);
    }

  }
}

} // namespace

OffsetTable OffsetTable::Build(const TypeInfoMap& tinfo_map, u8 ptr_size) {
  vector<TypeInfo> types;
  for (const auto& type_pair : tinfo_map.GetTypeMap()) {
    const auto& tinfo = type_pair.second;
    types.emplace_back(tinfo);
  }

  // Sort by the types' top-sort indices in ascending order.
  {
    auto lt_cmp = [&](const TypeInfo& lhs, const TypeInfo& rhs) {
      return lhs.top_sort_index < rhs.top_sort_index;
    };
    std::sort(types.begin(), types.end(), lt_cmp);
  }

  FieldMap field_offsets;
  TypeMap type_sizes;
  BuildTypeSizesAndFieldOffsets(types, ptr_size, &type_sizes, &field_offsets);

  MethodMap method_offsets;
  VtableMap vtables;
  BuildMethodOffsets(types, ptr_size, &method_offsets, &vtables);

  return OffsetTable(type_sizes, field_offsets, method_offsets, vtables, ptr_size);
}

} // namespace common
} // namespace backend
