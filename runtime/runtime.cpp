
#include "runtime/runtime.h"

namespace runtime {

const string TypeInfoFile =
#include "runtime/__joos_internal__/TypeInfo.java"
;

const string StringOpsFile =
#include "runtime/__joos_internal__/StringOps.java"
;

const string StackFrameFile =
#include "runtime/__joos_internal__/StackFrame.java"
;

const string ArrayFile =
#include "runtime/__joos_internal__/Array.java"
;

const int kNumRuntimeFiles = 4;

} // namespace runtime

