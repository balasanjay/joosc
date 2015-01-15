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

vector<int> FindLineStarts(const u8* buf, int len) {
  vector<int> newlines;
  newlines.push_back(0);

  for (int i = 0; i < len; ++i) {
    // TODO: handle windows-style newlines?
    if (buf[i] == '\n') {
      newlines.push_back(i + 1);
    }
  }
  return newlines;
}

}  // namespace

File::File(const string& path, u8* buf, int len)
    : dirname_(base::Dirname(path)),
      basename_(base::Basename(path)),
      buf_(buf),
      len_(len),
      linestarts_(FindLineStarts(buf_, len_)) {
  assert(buf != nullptr && len >= 0);
}

u8 File::At(int index) const {
  AssertInRange(index);

  return buf_[index];
}

void File::IndexToLineCol(int index, int* line_out, int* col_out) const {
  AssertInRange(index);

  // Binary search the newline vector, to find the largest newline that is <=
  // offset. std::upper_bound returns the first element > offset, which is
  // one-off from what we want.
  auto iter = std::upper_bound(linestarts_.begin(), linestarts_.end(), index);
  --iter;

  *line_out = iter - linestarts_.begin();
  *col_out = index - *iter;
}

inline void File::AssertInRange(int index) const {
  if (index < 0 || index >= len_) {
    std::stringstream ss;
    ss << "File::operator[] index " << index << " out of range [" << 0 << ", "
       << len_ << std::endl;
    throw ss.str();
  }
}

std::ostream& operator<<(std::ostream& out, const Pos& p) {
  return out << p.fileid << ":" << p.index;
}

std::ostream& operator<<(std::ostream& out, const PosRange& p) {
  // Handle single-element ranges specially.
  if (p.begin + 1 == p.end) {
    return out << p.fileid << ":" << p.begin;
  }

  return out << p.fileid << ":" << p.begin << "-" << p.end;
}

void PrintRangePtr(std::ostream* out, const File* file, const PosRange& pos) {
  const int MAX_CONTEXT = 40;

  // TODO(sjy): this will be slightly-broken for PosRanges that start on a
  // newline character. Figure out what to do here.

  // Walk backwards until we find a newline, so we know where to start printing.
  int begin = pos.begin;
  while (begin > 0 && pos.begin - begin < MAX_CONTEXT) {
    u8 c = file->At(begin - 1);
    if (c == '\n' || c == '\r') {
      break;
    }
    --begin;
  }

  // TODO(sjy): handle large ranges, by printing both the begining and the end
  // of the range with a "…" character in the middle.

  // Walk forwards until we find a newline, so we know where to start printing.
  // Note that we purposely start this at pos.begin, not pos.end. If the error
  // comprises an extremely large portion of text, we prefer to not spit it all
  // out here.
  int end = pos.begin;
  while (end < file->Size() && end - pos.begin < MAX_CONTEXT) {
    u8 c = file->At(end);
    if (c == '\n' || c == '\r') {
      break;
    }
    ++end;
  }

  // Now that we have both begin and end, we print the user's code.
  for (int i = begin; i < end; ++i) {
    u8 c = file->At(i);
    *out << c;
  }
  *out << '\n';

  // Finally, we print our pointer characters.
  for (int i = begin; i < end; ++i) {
    if (i == pos.begin) {
      *out << '^';
    } else if (pos.begin < i && i < pos.end) {
      *out << '~';
    } else {
      *out << ' ';
    }
  }
}

}  // namespace base
