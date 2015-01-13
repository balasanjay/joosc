#include "base/file.h"

#include <algorithm>

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

namespace {

vector<int> FindAllNewlines(const u8* buf, int len) {
  // TODO: handle windows-style newlines?
  vector<int> newlines;

  newlines.push_back(0);
  for (int i = 0; i < len; ++i) {
    if (buf[i] == '\n') {
      newlines.push_back(i+1);
    }
  }
  return newlines;
}

} // namespace

File::File(const string& path, u8* buf, int len)
  : dirname_(base::Dirname(path)),
    basename_(base::Basename(path)),
    buf_(buf),
    len_(len),
    newlines_(FindAllNewlines(buf_, len_)) {
    assert(buf != nullptr && len >= 0);
  }


u8 File::At(int index) const {
  if (index >= len_) {
    std::stringstream ss;
    ss << "File::operator[] index " << index << " out of range [" << 0 << ", "
       << len_ << std::endl;
    throw ss.str();
  }
  return buf_[index];
}

void File::OffsetToLineCol(int offset, int* line_out, int* col_out) const {
  int fakeline = -1;
  int fakecol = -1;

  if (line_out == nullptr) {
    line_out = &fakeline;
  }
  if (col_out == nullptr) {
    col_out = &fakecol;
  }

  // Binary search the newline vector, to find the largest newline that is <=
  // offset. std::upper_bound returns the first element > offset, which is
  // one-off from what we want.
  auto iter = std::upper_bound(newlines_.begin(), newlines_.end(), offset);
  --iter;

  *line_out = iter - newlines_.begin();
  *col_out = offset - *iter;
}

}  // namespace base
