#ifndef LEXER_ERROR_H
#define LEXER_ERROR_H

#include "base/error.h"
#include "base/macros.h"

namespace lexer {

class SimplePosRangeError : public base::Error {
  protected:
    SimplePosRangeError(base::FileSet* fs, PosRange posrange) : fs_(fs), posrange_(posrange) {}

    void PrintTo(std::ostream* out, const base::OutputOptions& opt) const override {
      if (opt.simple) {
        *out << SimpleError() << "(" << posrange_ << ")";
        return;
      }

      base::File* file = fs_->Get(posrange_.begin.fileid);

      *out << file->Dirname() << file->Basename() << ":"
        << posrange_.begin << ": " << Red(opt) << "error: " << ResetFmt(opt)
        << Error();

      // TODO: implement Pos to column:offset conversion.
      // TODO: implement pretty range-printing.
    }

    virtual string SimpleError() const = 0;
    virtual string Error() const = 0;

  private:
    base::FileSet* fs_;
    PosRange posrange_;
};

class NonAnsiCharError : public SimplePosRangeError {
  public:
    NonAnsiCharError(base::FileSet* fs, Pos pos) : SimplePosRangeError(fs, PosRange(pos)) {}

  protected:
    string SimpleError() const override { return "NonAnsiCharError"; }
    string Error() const override { return "Non-ANSI character"; }
};

class LeadingZeroInIntLitError : public SimplePosRangeError {
  public:
    LeadingZeroInIntLitError(base::FileSet* fs, Pos pos) : SimplePosRangeError(fs, PosRange(pos)) {}

  protected:
    string SimpleError() const override { return "LeadingZeroInIntLitError"; }
    string Error() const override { return "Cannot have leading '0' in non-zero integer literal."; }
};

class UnclosedBlockCommentError : public SimplePosRangeError {
  public:
    UnclosedBlockCommentError(base::FileSet* fs, PosRange posrange) : SimplePosRangeError(fs, posrange) {}

  protected:
    string SimpleError() const override { return "UnclosedBlockCommentError"; }
    string Error() const override { return "Unclosed block comment at end of file."; }
};

class UnclosedStringLitError : public SimplePosRangeError {
  public:
    UnclosedStringLitError(base::FileSet* fs, Pos pos) : SimplePosRangeError(fs, pos) {}

  protected:
    string SimpleError() const override { return "UnclosedStringLitError"; }
    string Error() const override { return "Unclosed string literal."; }
};


} // namespace lexer

#endif
