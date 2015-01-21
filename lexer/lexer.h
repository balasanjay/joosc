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
  XOR,
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
  K_BOOL,
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

struct TokenTypeInfo {
  static TokenTypeInfo New(TokenType type, string repr, string value) {
    TokenTypeInfo t;
    t.type_ = type;
    t.repr_ = repr;
    t.value_ = value;
    return t;
  }

  static TokenTypeInfo FromTokenType(TokenType type);

  TokenTypeInfo(const TokenTypeInfo& other) : type_(other.type_), kind_(other.kind_), supported_(other.supported_), precedence_(other.precedence_), repr_(other.repr_), value_(other.value_) {  }

  TokenTypeInfo Keyword() const {
    TokenTypeInfo ret = *this;
    ret.kind_ = Kind(ret.kind_ | KEYWORD);
    return ret;
  }

  TokenTypeInfo BinOp(int prec) const {
    TokenTypeInfo ret = *this;
    ret.kind_ = Kind(ret.kind_ | BIN_OP);
    ret.kind_ = Kind(ret.kind_ | SYMBOL);
    ret.precedence_ = prec;
    return ret;
  }

  TokenTypeInfo UnaryOp() const {
    TokenTypeInfo ret = *this;
    ret.kind_ = Kind(ret.kind_ | UNARY_OP);
    ret.kind_ = Kind(ret.kind_ | SYMBOL);
    return ret;
  }

  TokenTypeInfo Unsupported() const {
    TokenTypeInfo ret = *this;
    ret.kind_ = NONE;
    ret.supported_ = false;
    ret.precedence_ = -1;
    return ret;
  }

  TokenTypeInfo Symbol() const {
    TokenTypeInfo ret = *this;
    ret.kind_ = Kind(ret.kind_ | SYMBOL);
    return ret;
  }

  TokenTypeInfo Primitive() const {
    TokenTypeInfo ret = *this;
    ret.kind_ = Kind(ret.kind_ | PRIMITIVE);
    return ret;
  }

  TokenTypeInfo Literal() const {
    TokenTypeInfo ret = *this;
    ret.kind_ = Kind(ret.kind_ | LITERAL);
    return ret;
  }

  TokenType Type() const { return type_; }
  bool IsKeyword() const { return (kind_ & KEYWORD) == KEYWORD; }
  bool IsBinOp() const { return (uint(kind_) & uint(BIN_OP)) == uint(BIN_OP); }
  bool IsUnaryOp() const { return (kind_ & UNARY_OP) == UNARY_OP; }
  bool IsPrimitive() const { return (kind_ & PRIMITIVE) == PRIMITIVE; }
  bool IsLiteral() const { return (kind_ & LITERAL) == LITERAL; }
  bool IsSymbol() const { return (kind_ & SYMBOL) == SYMBOL; }

  int BinOpPrec() const {
    assert(IsBinOp());
    return precedence_;
  }

  string Value() const { return value_; }

private:
  enum Kind {
    NONE = 0,
    KEYWORD = 1 << 1,
    BIN_OP = 1 << 2,
    UNARY_OP = 1 << 3,
    PRIMITIVE = 1 << 4,
    LITERAL = 1 << 5,
    SYMBOL = 1 << 6,
  };

  friend std::ostream& operator<<(std::ostream& out, const TokenTypeInfo& t);

  TokenTypeInfo() {}

  TokenType type_ = NUM_TOKEN_TYPES;
  Kind kind_ = NONE;
  bool supported_ = true;
  int precedence_ = -1; // Only valid if kind_ == BIN_OP.
  string repr_ = "";
  string value_ = "";
};

std::ostream& operator<<(std::ostream& out, const TokenTypeInfo& t);

struct Token {
  Token(TokenType type, base::PosRange pos) : type(type), pos(pos) {}

  bool operator==(const Token& other) const {
    return type == other.type && pos == other.pos;
  }
  bool operator!=(const Token& other) const { return !(*this == other); }

  TokenTypeInfo TypeInfo() const;

  TokenType type;
  base::PosRange pos;
};

void LexJoosFiles(base::FileSet* fs, vector<vector<Token>>* tokens_out,
                  base::ErrorList* errors_out);

}  // namespace lexer

#endif
