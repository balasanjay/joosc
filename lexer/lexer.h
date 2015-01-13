#ifndef LEXER_LEXER_H
#define LEXER_LEXER_H

#include "base/errorlist.h"
#include "base/file.h"
#include "base/fileset.h"

namespace lexer {

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
  CHAR,
  STRING,
  NUM_TOKEN_TYPES,  // Not a valid token type.
};

string TokenTypeToString(TokenType t);

struct Token {
  Token(TokenType type, base::PosRange pos) : type(type), pos(pos) {}

  bool operator==(const Token& other) const {
    return type == other.type && pos == other.pos;
  }
  bool operator!=(const Token& other) const { return !(*this == other); }

  TokenType type;
  base::PosRange pos;
};

void LexJoosFiles(base::FileSet* fs, vector<vector<Token>>* tokens_out,
                  base::ErrorList* errors_out);

}  // namespace lexer

#endif
