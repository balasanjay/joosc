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
  INTEGER,
  IDENTIFIER,
  CHAR,
  STRING,
  // Keywords.
  K_ABSTRACT,
  K_DEFAULT,
  K_IF,
  K_PRIVATE,
  K_THIS,
  K_BOOLEAN,
  K_DO,
  K_IMPLEMENTS,
  K_PROTECTED,
  K_THROW,
  K_BREAK,
  K_DOUBLE,
  K_IMPORT,
  K_PUBLIC,
  K_THROWS,
  K_BYTE,
  K_ELSE,
  K_INSTANCEOF,
  K_RETURN,
  K_TRANSIENT,
  K_CASE,
  K_EXTENDS,
  K_INT,
  K_SHORT,
  K_TRY,
  K_CATCH,
  K_FINAL,
  K_INTERFACE,
  K_STATIC,
  K_VOID,
  K_CHAR,
  K_FINALLY,
  K_LONG,
  K_STRICTFP,
  K_VOLATILE,
  K_CLASS,
  K_FLOAT,
  K_NATIVE,
  K_SUPER,
  K_WHILE,
  K_CONST,
  K_FOR,
  K_NEW,
  K_SWITCH,
  K_CONTINUE,
  K_GOTO,
  K_PACKAGE,
  K_SYNCHRONIZED,
  K_TRUE,
  K_FALSE,
  K_NULL,
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

void LexPostProcess(base::FileSet* fs, vector<vector<Token>>* tokens_out, base::ErrorList* errors_out);

}  // namespace lexer

#endif
