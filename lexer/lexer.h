#ifndef LEXER_LEXER_H
#define LEXER_LEXER_H

#include "base/errorlist.h"
#include "base/file.h"
#include "base/fileset.h"

namespace lexer {

struct Pos {
  Pos(int fileid, int offset) : fileid(fileid), offset(offset) {}

  bool operator==(const Pos& other) const {
    return fileid == other.fileid && offset == other.offset;
  }
  bool operator!=(const Pos& other) const { return !(*this == other); }

  int fileid;
  int offset;
};

std::ostream& operator<<(std::ostream& out, const Pos& p);

struct PosRange {
  PosRange(int fileid, int begin, int end)
      : begin(fileid, begin), end(fileid, end) {}

  PosRange(const Pos& pos) : begin(pos), end(pos.fileid, pos.offset + 1) {}


  bool operator==(const PosRange& other) const {
    return begin == other.begin && end == other.end;
  }
  bool operator!=(const PosRange& other) const { return !(*this == other); }

  Pos begin;
  Pos end;
};

std::ostream& operator<<(std::ostream& out, const PosRange& p);

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
  ASSG,
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
  INTEGER,
  IDENTIFIER,
  STRING,
  NUM_TOKEN_TYPES,  // Not a valid token type.
};

string TokenTypeToString(TokenType t);

struct Token {
  Token(TokenType type, PosRange pos) : type(type), pos(pos) {}

  bool operator==(const Token& other) const {
    return type == other.type && pos == other.pos;
  }
  bool operator!=(const Token& other) const { return !(*this == other); }

  TokenType type;
  PosRange pos;
};

void LexJoosFiles(base::FileSet* fs, vector<vector<Token>>* tokens_out,
                  base::ErrorList* errors_out);

}  // namespace lexer

#endif
