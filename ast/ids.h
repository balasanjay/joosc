#ifndef AST_IDS_H
#define AST_IDS_H

namespace ast {

struct TypeId {
  using Base = u64;

  static const u64 kUnassignedBase = 0;
  static const u64 kErrorBase = 1;

  static TypeId Error() {
    return TypeId{kErrorBase, 0};
  }
  static TypeId Unassigned() {
    return TypeId{kUnassignedBase, 0};
  }

  bool IsUnassigned() const {
    return base == kUnassignedBase;
  }
  bool IsError() const {
    return base == kErrorBase;
  }

  Base base;
  u64 ndims;
};

} // namespace ast

#endif
