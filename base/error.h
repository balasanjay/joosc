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

  string Red() const;
  string Magenta() const;
  string DarkGray() const;
  string Green() const;
  string ResetColor() const;
  string BoldOn() const;
  string BoldOff() const;
};

class Error {
 public:
  virtual ~Error() = default;

  virtual void PrintTo(std::ostream* out, const OutputOptions& opt, const FileSet* fs) const = 0;

 protected:
  Error() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(Error);
};
std::ostream& operator<<(std::ostream& out, const Error& e);

using PrintFn = std::function<void(std::ostream*, const OutputOptions&, const FileSet*)>;
Error* MakeError(PrintFn printfn);

enum class DiagnosticClass {
  ERROR,
  WARNING,
  INFO,
};

void PrintDiagnosticHeader(std::ostream* out, const OutputOptions& opt,
                           const FileSet* fs, PosRange pos, DiagnosticClass cls,
                           string msg);
void PrintRangePtr(std::ostream* out, const OutputOptions& opt,
                   const FileSet* fs, const PosRange& pos);

// DEPRECATED; use MakeSimplePosRange error or subclass Error directly.
// TODO: delete all uses of this in lexer_error.h, and delete this class.
class PosRangeError : public Error {
 protected:
  PosRangeError(PosRange posrange)
      : posrange_(posrange) {}

 public:
  void PrintTo(std::ostream* out, const OutputOptions& opt, const FileSet* fs) const override {
    if (opt.simple) {
      *out << SimpleError() << "(" << posrange_ << ")";
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, posrange_, DiagnosticClass::ERROR,
                          Error());
    PrintRangePtr(out, opt, fs, posrange_);
  }

 protected:
  virtual string SimpleError() const = 0;
  virtual string Error() const = 0;

 private:
  PosRange posrange_;
};

Error* MakeSimplePosRangeError(PosRange pos, string name, string msg);

}  // namespace base

#endif
