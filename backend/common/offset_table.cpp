#include "backend/common/offset_table.h"

#include <algorithm>

#include "ast/ids.h"
#include "base/algorithm.h"
#include "base/printf.h"
#include "ir/size.h"

using std::get;
using std::make_tuple;
using std::tie;
using std::tuple;

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
using types::MethodSignature;
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

void BuildClassMethodOffsets(const vector<TypeInfo>& types, u8 ptr_size,
    OffsetTable::MethodMap* method_out, OffsetTable::VtableMap* vtable_out) {
  map<TypeId, u64> starting_offsets;

  using Vpair = pair<TypeId, MethodId>;
  auto vtable_cmp = [&](const Vpair& lhs, const Vpair& rhs) {
    return method_out->at(lhs.second).first < method_out->at(rhs.second).first;
  };

  for (const TypeInfo& tinfo : types) {
    // Interface methods will use a different lookup.
    if (tinfo.kind == TypeKind::INTERFACE) {
      continue;
    }

    u64 starting_offset = OffsetTable::VtableOverhead(ptr_size);
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

      method_out->insert({minfo.mid, {my_offset, TypeKind::CLASS}});
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

void BuildIfaceMethodOffsets(const vector<TypeInfo>& types, u8 ptr_size,
    OffsetTable::MethodMap* method_out, OffsetTable::ItableMap* itable_out) {

  using Ituple = tuple<u64, TypeId, MethodId>;
  auto itable_cmp = [&](const Ituple& lhs, const Ituple& rhs) {
    return get<0>(lhs) < get<0>(rhs);
  };

  // Pass 1: we collect all interface method signatures.
  map<MethodSignature, u64> iface_methods;
  for (const TypeInfo& tinfo : types) {
    if (tinfo.kind != TypeKind::INTERFACE) {
      continue;
    }

    for (const auto& m_pair : tinfo.methods.GetMethodMap()) {
      const MethodInfo& minfo = m_pair.second;

      // All interfaces inherit from Object, but these methods do not require
      // going through the itable map. They will simply go through the regular
      // vtable map. To avoid overriding Object's vtable-mapping, we skip any
      // such interface methods.
      {
        auto iter = method_out->find(minfo.mid);
        if (iter != method_out->end()) {
          continue;
        }
      }

      iface_methods.insert({minfo.signature, 0});
    }
  }

  // Pass 2: we assign all interface method signatures an offset.
  u64 offset = 0;
  for (auto& p : iface_methods) {
    p.second = offset;
    offset += (u64)ptr_size;
  }

  // Pass 3: we build a mapping from interface method id to offset.
  for (const TypeInfo& tinfo : types) {
    if (tinfo.kind != TypeKind::INTERFACE) {
      continue;
    }

    for (const auto& m_pair : tinfo.methods.GetMethodMap()) {
      const MethodInfo& minfo = m_pair.second;

      // Ignore inherited methods.
      if (tinfo.type != minfo.class_type) {
        continue;
      }

      u64 offset = 0;
      {
        auto iter = iface_methods.find(minfo.signature);
        if (iter == iface_methods.end()) {
          continue;
        }
        offset = iter->second;
      }

      {
        auto iter = method_out->insert({minfo.mid, {offset, TypeKind::INTERFACE}});
        // Either there wasn't an entry with this key, or there was. Either
        // way, check that it got the offset we computed.
        CHECK(iter.first->second.first == offset);
      }
    }
  }

  // Pass 4: we build an itable for each class.
  for (const TypeInfo& tinfo : types) {
    if (tinfo.kind != TypeKind::CLASS) {
      continue;
    }
    OffsetTable::Itable itable;
    for (const auto& m_pair : tinfo.methods.GetMethodMap()) {
      const MethodInfo& minfo = m_pair.second;
      auto iter = iface_methods.find(minfo.signature);
      if (iter == iface_methods.end()) {
        continue;
      }
      itable.emplace_back(make_tuple(iter->second, minfo.class_type, minfo.mid));
    }

    std::sort(itable.begin(), itable.end(), itable_cmp);
    {
      auto iter = itable_out->insert({tinfo.type, itable});
      CHECK(iter.second);
    }
  }
}

void BuildStaticFieldMap(const vector<TypeInfo>& types,
    OffsetTable::StaticFieldMap* out) {
  for (const TypeInfo& tinfo : types) {
    // Joos interfaces don't have fields.
    if (tinfo.kind == TypeKind::INTERFACE) {
      continue;
    }

    OffsetTable::StaticFields fields;
    for (const auto& f_pair: tinfo.fields.GetFieldMap()) {
      const FieldInfo& finfo = f_pair.second;
      if (!finfo.mods.HasModifier(lexer::STATIC)) {
        continue;
      }

      // Skip static fields that were pushed down from the parent.
      if (finfo.class_type != tinfo.type) {
        continue;
      }

      fields.push_back({finfo.fid, SizeClassFrom(finfo.field_type)});
    }

    // Static runtime TypeInfo field.
    fields.push_back({ast::kStaticTypeInfoId, SizeClass::PTR});

    {
      using Fpair = pair<FieldId, SizeClass>;
      auto cmp = [](const Fpair& lhs, const Fpair& rhs) {
        return lhs.second > rhs.second;
      };
      std::stable_sort(fields.begin(), fields.end(), cmp);
    }

    auto iter = out->insert({tinfo.type, fields});
    CHECK(iter.second);
  }
}
void BuildNatives(const vector<TypeInfo>& types,
    OffsetTable::NativeMap* out) {
  for (const auto& type : types) {
    for (const auto& m_pair : type.methods.GetMethodMap()) {
      const MethodInfo& minfo = m_pair.second;

      // Ignore non-native methods.
      if (!minfo.mods.HasModifier(lexer::NATIVE)) {
        continue;
      }

      // Ignore inherited native methods.
      if (minfo.class_type != type.type) {
        continue;
      }

      stringstream ss;
      ss << "NATIVE" << type.package << "." << type.name << "." << minfo.signature.name;
      auto iter = out->insert({minfo.mid, ss.str()});
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

  StaticFieldMap statics;
  BuildStaticFieldMap(types, &statics);

  MethodMap method_offsets;

  VtableMap vtables;
  BuildClassMethodOffsets(types, ptr_size, &method_offsets, &vtables);

  ItableMap itables;
  BuildIfaceMethodOffsets(types, ptr_size, &method_offsets, &itables);

  NativeMap natives;
  BuildNatives(types, &natives);

  return OffsetTable(type_sizes, field_offsets, method_offsets, vtables, itables, statics, natives, ptr_size);
}

} // namespace common
} // namespace backend
