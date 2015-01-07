
#include "base/file_impl.h"
#include <cstring>

namespace base {

  u8* StringCopy(std::string s) {
    u8* buf = new u8[s.length()];
    memmove(buf, s.c_str(), s.length());
    return buf;
  }

  StringFile::StringFile(std::string s):
      File(StringCopy(s), s.length()) {
  }

  StringFile::~StringFile() {
    delete[] buf_;
  }


} // namespace base

