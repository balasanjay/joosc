#ifndef BASE_FILE_H
#define BASE_FILE_H

#include <string>
#include <sstream>
#include <vector>
#include <cassert>

namespace base {

// TODO: comment me.
string Dirname(string path);
// TODO: comment me.
string Basename(string path);

class File {
public:
  u8 At(int index) const;

  const string& Dirname() {
    return dirname_;
  }

  const string& Basename() {
    return basename_;
  }

  int Size() const {
    return len_;
  }

protected:
  friend class FileSet;

  File(const string& path, u8* buf, int len): dirname_(base::Dirname(path)), basename_(base::Basename(path)), buf_(buf), len_(len) {
    assert(buf != nullptr && len >= 0);
  }
  virtual ~File() {}

  const string dirname_;
  const string basename_;

  const u8* buf_;
  const int len_;
};



} // namespace base

#endif
