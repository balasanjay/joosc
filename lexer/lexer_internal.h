#ifndef LEXER_LEXER_INTERNAL_H
#define LEXER_LEXER_INTERNAL_H

#include "lexer/lexer.h"

namespace lexer {
namespace internal {

const string kTokenTypeToString[NUM_TOKEN_TYPES] = {
  "LINE_COMMENT",
  "BLOCK_COMMENT",
  "WHITESPACE",
  "LE",
  "GE",
  "EQ",
  "NEQ",
  "AND",
  "OR",
  "ADD",
  "SUB",
  "MUL",
  "DIV",
  "MOD",
  "LT",
  "GT",
  "BAND",
  "BOR",
  "NOT",
  "ASSG",
  "LPAREN",
  "RPAREN",
  "LBRACE",
  "RBRACE",
  "LBRACK",
  "RBRACK",
  "SEMI",
  "COMMA",
  "DOT",
  "IF",
  "WHILE",
  "INTEGER",
  "IDENTIFIER",
  "CHAR",
  "STRING"
};

const int kNumSymbolLiterals = 26;
const pair<string, TokenType> kSymbolLiterals[kNumSymbolLiterals] = {
    make_pair("<=", LE),    make_pair(">=", GE),    make_pair("==", EQ),
    make_pair("!=", NEQ),   make_pair("&&", AND),   make_pair("||", OR),
    make_pair("+", ADD),    make_pair("-", SUB),    make_pair("*", MUL),
    make_pair("/", DIV),    make_pair("%", MOD),    make_pair("<", LT),
    make_pair(">", GT),     make_pair("&", BAND),   make_pair("|", BOR),
    make_pair("!", NOT),    make_pair("=", ASSG),   make_pair("(", LPAREN),
    make_pair(")", RPAREN), make_pair("{", LBRACE), make_pair("}", RBRACE),
    make_pair("[", LBRACK), make_pair("]", RBRACK), make_pair(";", SEMI),
    make_pair(",", COMMA),  make_pair(".", DOT)};

}  // namespace internal
}  // namespace lexer

#endif
