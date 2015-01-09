#ifndef LEXER_LEXER_H
#define LEXER_LEXER_H

#include "base/file.h"
#include "base/fileset.h"

namespace lexer {

struct Pos {
  Pos(int fileid, int offset) : fileid(fileid), offset(offset) {}

  int fileid;
  int offset;
};

struct PosRange {
  PosRange(int fileid, int begin, int end)
      : begin(fileid, begin), end(fileid, end) {}

  Pos begin;
  Pos end;
};

enum TokenType {
  LINE_COMMENT,
  BLOCK_COMMENT,
  WHITESPACE,
  LE,
  GE,
  EQ,
  NEQ,
  AND,
  OR,
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  LT,
  GT,
  BAND,
  BOR,
  NOT,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  LBRACK,
  RBRACK,
  SEMI,
  COMMA,
  DOT,
  IF,
  WHILE,
  NUM_TOKEN_TYPES,  // Not a valid token type.
};

string TokenTypeToString(TokenType t);

struct Token {
  Token(TokenType type, PosRange pos) : type(type), pos(pos) {}

  TokenType type;
  PosRange pos;
};

enum ErrorType {
  NON_ANSI_CHAR,
  NUM_ERROR_TYPES,  // Not a valid error type.
};

struct Error {
  Error(ErrorType type, PosRange pos) : type(type), pos(pos) {}

  ErrorType type;
  PosRange pos;
};

class UnterminatedStringLiteralError : public Error {};

void LexJoosFiles(base::FileSet* fs, vector<vector<Token>>* tokens_out,
                  vector<Error>* errors_out);

}  // namespace lexer

#endif
