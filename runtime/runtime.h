#ifndef RUNTIME_RUNTIME_H
#define RUNTIME_RUNTIME_H

#include "base/fileset.h"
#include "types/type_info_map.h"

namespace runtime {

base::FileSet* GenerateRuntimeFiles(const types::TypeInfoMap& tinfo_map);

} // namespace runtime

#endif

