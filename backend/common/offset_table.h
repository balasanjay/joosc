#ifndef BACKEND_COMMON_OFFSET_TABLE_H
#define BACKEND_COMMON_OFFSET_TABLE_H

#include "ast/ids.h"
#include "types/type_info_map.h"

namespace backend {
namespace common {

class OffsetTable {
public:
  static OffsetTable Build(const types::TypeInfoMap& tinfo_map, u8 ptr_size);

  u64 SizeOf(ast::TypeId tid) const {
    CHECK(tid.ndims == 0);
    return type_sizes_.at(tid) + ObjectOverhead();
  }

  u64 OffsetOf(ast::FieldId fid) const {
    return field_offsets_.at(fid) + ObjectOverhead();
  }

private:
  OffsetTable(const map<ast::TypeId, u64>& type_sizes, const map<ast::FieldId, u64>& field_offsets, u8 ptr_size) : type_sizes_(type_sizes), field_offsets_(field_offsets), ptr_size_(ptr_size) {}

  u64 ObjectOverhead() const {
    // vtable pointer.
    return (u64)ptr_size_;
  }

  map<ast::TypeId, u64> type_sizes_;
  map<ast::FieldId, u64> field_offsets_;
  u8 ptr_size_;
};

} // namespace common
} // namespace backend

#endif
