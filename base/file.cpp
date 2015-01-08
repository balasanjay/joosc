#include "base/file.h"

namespace base {

string Dirname(string path) {
  size_t result = path.find_last_of('/');
  if (result == string::npos) {
    return "";
  }

  return path.substr(0, result);
}

string Basename(string path) {
  size_t result = path.find_last_of('/');
  if (result == string::npos) {
    return path;
  }

  return path.substr(result + 1);
}

u8 File::At(int index) const {
  if (index >= len_) {
    std::stringstream ss;
    ss << "File::operator[] index " << index
       << " out of range [" << 0 << ", " << len_
       << std::endl;
    throw ss.str();
  }
  return buf_[index];
}

} // namespace base
