#ifndef BASE_FILE_WALKER_H
#define BASE_FILE_WALKER_H

#include <dirent.h>

#include "std.h"

namespace base {

bool WalkDir(const string& name, std::function<bool(const dirent& ent)> ent);

} // namespace base

#endif
