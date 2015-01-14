#ifndef LEXER_ERROR_H
#define LEXER_ERROR_H

#include "base/file.h"
#include "base/fileset.h"
#include "lexer/lexer.h"
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


      base::PrintRangePtr(out, file, posrange_);

      // TODO: implement Pos to column:offset conversion.
      // TODO: implement pretty range-printing.
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
    string Error() const override { return "Cannot have non-ANSI character."; }
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
    string Error() const override { return "Unclosed block comment."; }
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

class InvalidCharacterEscapeError : public SimplePosRangeError {
public:
  InvalidCharacterEscapeError(base::FileSet* fs, base::PosRange posrange) : SimplePosRangeError(fs, posrange) {}
protected:
  string SimpleError() const override { return "InvalidCharacterEscapeError"; }
  string Error() const override { return "Invalid character escape."; }
};

class InvalidCharacterLitError : public SimplePosRangeError {
public:
  InvalidCharacterLitError(base::FileSet* fs, base::PosRange posrange) : SimplePosRangeError(fs, posrange) {}
protected:
  string SimpleError() const override { return "InvalidCharacterLitError"; }
  string Error() const override { return "Invalid character literal."; }
};

} // namespace lexer

#endif
