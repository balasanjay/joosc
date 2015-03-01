#include "ast/ids.h"

namespace ast {

const TypeId TypeId::kUnassigned = {TypeId::kUnassignedBase, 0};
const TypeId TypeId::kError = {TypeId::kErrorBase, 0};
const TypeId TypeId::kNull = {TypeId::kNullBase, 0};
const TypeId TypeId::kType = {TypeId::kTypeBase, 0};
const TypeId TypeId::kVoid = {TypeId::kVoidBase, 0};
const TypeId TypeId::kBool = {TypeId::kBoolBase, 0};
const TypeId TypeId::kByte = {TypeId::kByteBase, 0};
const TypeId TypeId::kChar = {TypeId::kCharBase, 0};
const TypeId TypeId::kShort = {TypeId::kShortBase, 0};
const TypeId TypeId::kInt = {TypeId::kIntBase, 0};

} // namespace ast

