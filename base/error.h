#ifndef BASE_ERROR_H
#define BASE_ERROR_H

#include <ostream>
#include "base/fileset.h"

namespace base {

struct OutputOptions {
  OutputOptions(bool colorize, bool simple)
      : colorize(colorize), simple(simple) {}

  const bool colorize = true;
  const bool simple = false;

  static const OutputOptions kSimpleOutput;
  static const OutputOptions kUserOutput;
};

class Error {
 public:
  virtual ~Error() = default;

  virtual void PrintTo(std::ostream* out, const OutputOptions& opt) const = 0;

 protected:
  Error() = default;

  string Red(const OutputOptions& opt) const;
  string ResetFmt(const OutputOptions& opt) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(Error);
};

std::ostream& operator<<(std::ostream& out, const Error& e);

class PosRangeError : public Error {
 protected:
  PosRangeError(const FileSet* fs, PosRange posrange)
      : fs_(fs), posrange_(posrange) {}

 public:
  void PrintTo(std::ostream* out, const OutputOptions& opt) const override {
    if (opt.simple) {
      *out << SimpleError() << "(" << posrange_ << ")";
      return;
    }

    const File* file = fs_->Get(posrange_.fileid);

    // Get line and column info.
    int line = -1;
    int col = -1;
    file->IndexToLineCol(posrange_.begin, &line, &col);

    *out << file->Dirname() << file->Basename() << ":" << line + 1 << ":"
         << col + 1 << ": " << Red(opt) << "error: " << ResetFmt(opt) << Error()
         << '\n';

    PrintRangePtr(out, file, posrange_);
  }

 protected:
  virtual string SimpleError() const = 0;
  virtual string Error() const = 0;

 private:
  const FileSet* fs_;
  PosRange posrange_;
};

Error* MakeSimplePosRangeError(const FileSet* fs, PosRange pos, string name,
                               string msg);

}  // namespace base

#endif
