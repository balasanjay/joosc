#ifndef BASE_FILE_H
#define BASE_FILE_H

#include <string>
#include <vector>

namespace base {

// TODO: comment me.
string Dirname(string path);
// TODO: comment me.
string Basename(string path);

// TODO: comment me.
class File {
 public:
  // TODO: comment me.
  int Size() const { return len_; }
  // TODO: comment me.
  u8 At(int index) const;

  // Converts a zero-based index to a zero-based line number and a zero-based
  // column number.  Takes time logarithmic in the size of the file.
  void IndexToLineCol(int offset, int* line_out, int* col_out) const;

  // TODO: comment me.
  const string& Dirname() const { return dirname_; }
  // TODO: comment me.
  const string& Basename() const { return basename_; }

 protected:
  friend class FileSet;

  File(const string& path, u8* buf, int len);

  virtual ~File() {}

  // Asserts that the provided index is a valid position in buf_.
  inline void AssertInRange(int index) const;

  const string dirname_;
  const string basename_;

  const u8* buf_;
  const int len_;

  const vector<int> linestarts_;  // Stored as indices into buf_.

 private:
  DISALLOW_COPY_AND_ASSIGN(File);
};

struct Pos {
  Pos(int fileid, int index) : fileid(fileid), index(index) {}

  bool operator==(const Pos& other) const {
    return fileid == other.fileid && index == other.index;
  }
  bool operator!=(const Pos& other) const { return !(*this == other); }

  int fileid;
  int index;
};

std::ostream& operator<<(std::ostream& out, const Pos& p);

struct PosRange {
  PosRange(int fileid, int begin, int end)
      : fileid(fileid), begin(begin), end(end){};
  PosRange(const Pos& pos)
      : fileid(pos.fileid), begin(pos.index), end(pos.index + 1) {}

  bool operator==(const PosRange& other) const {
    return fileid == other.fileid && begin == other.begin && end == other.end;
  }
  bool operator!=(const PosRange& other) const { return !(*this == other); }

  int fileid;
  int begin;
  int end;
};

std::ostream& operator<<(std::ostream& out, const PosRange& p);

}  // namespace base

#endif
