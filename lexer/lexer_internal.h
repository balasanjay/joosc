#ifndef LEXER_LEXER_INTERNAL_H
#define LEXER_LEXER_INTERNAL_H

#include "lexer/lexer.h"

namespace lexer {
namespace internal {

const string kTokenTypeToString[NUM_TOKEN_TYPES] = {
    "LINE_COMMENT",   "BLOCK_COMMENT", "WHITESPACE",  "LE",         "GE",
    "EQ",             "NEQ",           "AND",         "OR",         "ADD",
    "SUB",            "MUL",           "DIV",         "MOD",        "LT",
    "GT",             "BAND",          "BOR",         "NOT",        "ASSG",
    "LPAREN",         "RPAREN",        "LBRACE",      "RBRACE",     "LBRACK",
    "RBRACK",         "SEMI",          "COMMA",       "DOT",        "INTEGER",
    "IDENTIFIER",     "CHAR",          "STRING",      "K_ABSTRACT", "K_DEFAULT",
    "K_IF",           "K_PRIVATE",     "K_THIS",      "K_BOOLEAN",  "K_DO",
    "K_IMPLEMENTS",   "K_PROTECTED",   "K_THROW",     "K_BREAK",    "K_DOUBLE",
    "K_IMPORT",       "K_PUBLIC",      "K_THROWS",    "K_BYTE",     "K_ELSE",
    "K_INSTANCEOF",   "K_RETURN",      "K_TRANSIENT", "K_CASE",     "K_EXTENDS",
    "K_INT",          "K_SHORT",       "K_TRY",       "K_CATCH",    "K_FINAL",
    "K_INTERFACE",    "K_STATIC",      "K_VOID",      "K_CHAR",     "K_FINALLY",
    "K_LONG",         "K_STRICTFP",    "K_VOLATILE",  "K_CLASS",    "K_FLOAT",
    "K_NATIVE",       "K_SUPER",       "K_WHILE",     "K_CONST",    "K_FOR",
    "K_NEW",          "K_SWITCH",      "K_CONTINUE",  "K_GOTO",     "K_PACKAGE",
    "K_SYNCHRONIZED", "K_TRUE",        "K_FALSE",     "K_NULL"};

const int kNumKeywordLiterals = 51;
const pair<string, TokenType> kKeywordLiterals[kNumKeywordLiterals] = {
    make_pair("abstract", K_ABSTRACT),
    make_pair("default", K_DEFAULT),
    make_pair("if", K_IF),
    make_pair("private", K_PRIVATE),
    make_pair("this", K_THIS),
    make_pair("boolean", K_BOOLEAN),
    make_pair("do", K_DO),
    make_pair("implements", K_IMPLEMENTS),
    make_pair("protected", K_PROTECTED),
    make_pair("throw", K_THROW),
    make_pair("break", K_BREAK),
    make_pair("double", K_DOUBLE),
    make_pair("import", K_IMPORT),
    make_pair("public", K_PUBLIC),
    make_pair("throws", K_THROWS),
    make_pair("byte", K_BYTE),
    make_pair("else", K_ELSE),
    make_pair("instanceof", K_INSTANCEOF),
    make_pair("return", K_RETURN),
    make_pair("transient", K_TRANSIENT),
    make_pair("case", K_CASE),
    make_pair("extends", K_EXTENDS),
    make_pair("int", K_INT),
    make_pair("short", K_SHORT),
    make_pair("try", K_TRY),
    make_pair("catch", K_CATCH),
    make_pair("final", K_FINAL),
    make_pair("interface", K_INTERFACE),
    make_pair("static", K_STATIC),
    make_pair("void", K_VOID),
    make_pair("char", K_CHAR),
    make_pair("finally", K_FINALLY),
    make_pair("long", K_LONG),
    make_pair("strictfp", K_STRICTFP),
    make_pair("volatile", K_VOLATILE),
    make_pair("class", K_CLASS),
    make_pair("float", K_FLOAT),
    make_pair("native", K_NATIVE),
    make_pair("super", K_SUPER),
    make_pair("while", K_WHILE),
    make_pair("const", K_CONST),
    make_pair("for", K_FOR),
    make_pair("new", K_NEW),
    make_pair("switch", K_SWITCH),
    make_pair("continue", K_CONTINUE),
    make_pair("goto", K_GOTO),
    make_pair("package", K_PACKAGE),
    make_pair("synchronized", K_SYNCHRONIZED),
    make_pair("true", K_TRUE),
    make_pair("false", K_FALSE),
    make_pair("null", K_NULL)};

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
