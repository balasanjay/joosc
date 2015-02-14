#ifndef AST_IDS_H
#define AST_IDS_H

namespace ast {

struct TypeId {
  using Base = u64;

  static const Base kUnassignedBase = 0;
  static const Base kErrorBase = 1;
  static const Base kNullBase = 2;
  static const Base kVoidBase = 3;
  static const Base kBoolBase = 4;
  static const Base kByteBase = 5;
  static const Base kCharBase = 6;
  static const Base kShortBase = 7;
  static const Base kIntBase = 8;

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

  bool operator==(const TypeId& other) const {
    return base == other.base && ndims == other.ndims;
  }
  bool operator!=(const TypeId& other) const {
    return !(*this == other);
  }

  Base base;
  u64 ndims;
};

} // namespace ast

#endif
