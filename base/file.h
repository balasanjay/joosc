#ifndef BASE_FILE_H
#define BASE_FILE_H

#include <string>
#include <vector>

namespace base {

class File {
public:
  u8 operator[](i64 index) {
    // TODO: Assert that index < len_.
    return buf_[index];
  }

protected:
  friend class FileSet;

  File(u8* buf, i64 len): buf_(buf), len_(len) {
    // TODO: Check that len and buf are valid.
  }
  virtual ~File() {}

  const u8* buf_;
  const i64 len_;
};

class FileSet {

};

} // namespace base

#endif
