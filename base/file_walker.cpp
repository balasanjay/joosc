#include "base/file_walker.h"

#include <dirent.h>

namespace base {

bool WalkDir(const string& name, std::function<bool(const dirent& name)> cb) {
  auto del = [](DIR* dir) { closedir(dir); };

  uptr<DIR, decltype(del)> dir(opendir(name.c_str()), del);
  if (dir == nullptr) {
    return false;
  }

  while (true) {
    struct dirent* ent = readdir(dir.get());
    if (ent == nullptr) {
      break;
    }
    if (!cb(*ent)) {
      return false;
    }
  }
  return true;
}

} // namespace base
