#ifndef LEXER_ERROR_H
#define LEXER_ERROR_H

#include "base/file.h"
#include "base/fileset.h"
#include "lexer/lexer.h"
#include "base/error.h"
#include "base/macros.h"

namespace lexer {

class NonAnsiCharError : public base::PosRangeError {
 public:
  NonAnsiCharError(base::FileSet* fs, base::PosRange pos)
      : base::PosRangeError(fs, base::PosRange(pos)) {}

 protected:
  string SimpleError() const override { return "NonAnsiCharError"; }
  string Error() const override { return "Cannot have non-ANSI character."; }
};

class LeadingZeroInIntLitError : public base::PosRangeError {
 public:
  LeadingZeroInIntLitError(base::FileSet* fs, base::PosRange pos)
      : base::PosRangeError(fs, base::PosRange(pos)) {}

 protected:
  string SimpleError() const override { return "LeadingZeroInIntLitError"; }
  string Error() const override {
    return "Cannot have leading '0' in non-zero integer literal.";
  }
};

class UnclosedBlockCommentError : public base::PosRangeError {
 public:
  UnclosedBlockCommentError(base::FileSet* fs, base::PosRange posrange)
      : base::PosRangeError(fs, posrange) {}

 protected:
  string SimpleError() const override { return "UnclosedBlockCommentError"; }
  string Error() const override { return "Unclosed block comment."; }
};

class UnclosedStringLitError : public base::PosRangeError {
 public:
  UnclosedStringLitError(base::FileSet* fs, base::PosRange pos)
      : base::PosRangeError(fs, pos) {}

 protected:
  string SimpleError() const override { return "UnclosedStringLitError"; }
  string Error() const override { return "Unclosed string literal."; }
};

class UnexpectedCharError : public base::PosRangeError {
 public:
  UnexpectedCharError(base::FileSet* fs, base::PosRange pos)
      : base::PosRangeError(fs, pos) {}

 protected:
  string SimpleError() const override { return "UnexpectedCharError"; }
  string Error() const override { return "Unexpected character found."; }
};

class InvalidCharacterEscapeError : public base::PosRangeError {
 public:
  InvalidCharacterEscapeError(base::FileSet* fs, base::PosRange posrange)
      : base::PosRangeError(fs, posrange) {}

 protected:
  string SimpleError() const override { return "InvalidCharacterEscapeError"; }
  string Error() const override { return "Invalid character escape."; }
};

class InvalidCharacterLitError : public base::PosRangeError {
 public:
  InvalidCharacterLitError(base::FileSet* fs, base::PosRange posrange)
      : base::PosRangeError(fs, posrange) {}

 protected:
  string SimpleError() const override { return "InvalidCharacterLitError"; }
  string Error() const override { return "Invalid character literal."; }
};

class UnsupportedTokenError : public base::PosRangeError {
 public:
  UnsupportedTokenError(const base::FileSet* fs, base::PosRange posrange)
      : base::PosRangeError(fs, posrange) {}

 protected:
  string SimpleError() const override { return "UnsupportedTokenError"; }
  string Error() const override { return "Unsupported token."; }
};


}  // namespace lexer

#endif
