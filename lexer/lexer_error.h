#ifndef LEXER_ERROR_H
#define LEXER_ERROR_H

#include "base/error.h"
#include "base/macros.h"

namespace lexer {

class SimplePosRangeError : public base::Error {
  protected:
    SimplePosRangeError(base::FileSet* fs, base::PosRange posrange) : fs_(fs), posrange_(posrange) {}

    void PrintTo(std::ostream* out, const base::OutputOptions& opt) const override {
      if (opt.simple) {
        *out << SimpleError() << "(" << posrange_ << ")";
        return;
      }

      const base::File* file = fs_->Get(posrange_.fileid);

      // Get line and column info.
      int line = -1;
      int col = -1;
      file->IndexToLineCol(posrange_.begin, &line, &col);

      *out << file->Dirname() << file->Basename() << ":"
        << line + 1 << ":" << col + 1 << ": "
        << Red(opt) << "error: " << ResetFmt(opt) << Error() << '\n';


      PrettyPrintRange(out, file, posrange_, col);

      // TODO: implement Pos to column:offset conversion.
      // TODO: implement pretty range-printing.
    }

    void PrettyPrintRange(std::ostream* out, const base::File* file, const base::PosRange& range, int col) const {
      const int MAX_CONTEXT = 40;

      int begin = std::max(range.begin - MAX_CONTEXT, range.begin - col);
      int end = range.begin + MAX_CONTEXT;

      for (int i = begin; i < end; ++i) {
        u8 c = file->At(i);
        if (c == '\n') {
          end = i;
          break;
        }

        *out << c;
      }
      *out << '\n';

      for (int i = begin; i < end; ++i) {
        if (i == range.begin) {
          *out << '^';
        } else if (range.begin < i && i < range.end) {
          *out << '~';
        } else {
          *out << ' ';
        }
      }
    }

    virtual string SimpleError() const = 0;
    virtual string Error() const = 0;

  private:
    base::FileSet* fs_;
    base::PosRange posrange_;
};

class NonAnsiCharError : public SimplePosRangeError {
  public:
    NonAnsiCharError(base::FileSet* fs, base::PosRange pos) : SimplePosRangeError(fs, base::PosRange(pos)) {}

  protected:
    string SimpleError() const override { return "NonAnsiCharError"; }
    string Error() const override { return "Non-ANSI character"; }
};

class LeadingZeroInIntLitError : public SimplePosRangeError {
  public:
    LeadingZeroInIntLitError(base::FileSet* fs, base::PosRange pos) : SimplePosRangeError(fs, base::PosRange(pos)) {}

  protected:
    string SimpleError() const override { return "LeadingZeroInIntLitError"; }
    string Error() const override { return "Cannot have leading '0' in non-zero integer literal."; }
};

class UnclosedBlockCommentError : public SimplePosRangeError {
  public:
    UnclosedBlockCommentError(base::FileSet* fs, base::PosRange posrange) : SimplePosRangeError(fs, posrange) {}

  protected:
    string SimpleError() const override { return "UnclosedBlockCommentError"; }
    string Error() const override { return "Unclosed block comment at end of file."; }
};

class UnclosedStringLitError : public SimplePosRangeError {
  public:
    UnclosedStringLitError(base::FileSet* fs, base::PosRange pos) : SimplePosRangeError(fs, pos) {}

  protected:
    string SimpleError() const override { return "UnclosedStringLitError"; }
    string Error() const override { return "Unclosed string literal."; }
};

class UnexpectedCharError : public SimplePosRangeError {
  public:
    UnexpectedCharError(base::FileSet* fs, base::PosRange pos) : SimplePosRangeError(fs, pos) {}

  protected:
    string SimpleError() const override { return "UnexpectedCharError"; }
    string Error() const override { return "Unexpected character found."; }
};

} // namespace lexer

#endif
