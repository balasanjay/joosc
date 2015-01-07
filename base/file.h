#ifndef BASE_FILE_H
#define BASE_FILE_H

#include <string>
#include <sstream>
#include <vector>
#include <cassert>

namespace base {

class File {
public:
  u8 operator[](int index) const {
    if (index >= len_) {
      std::stringstream ss;
      ss << "File::operator[] index " << index
         << " out of range [" << 0 << ", " << len_
         << std::endl;
      throw ss.str();
    }
    return buf_[index];
  }

  int length() const {
    return len_;
  }

protected:
  friend class FileSet;

  File(u8* buf, int len): buf_(buf), len_(len) {
    assert(buf != nullptr && len >= 0);
  }
  virtual ~File() {}

  const u8* buf_;
  const int len_;
};

class FileSet {

};

} // namespace base

#endif
