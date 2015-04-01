
#include "runtime/runtime.h"

namespace runtime {

const string TypeInfoFile =
#include "runtime/__joos_internal__/TypeInfo.java"
;

const string StringOpsFile =
#include "runtime/__joos_internal__/StringOps.java"
;

} // namespace runtime

