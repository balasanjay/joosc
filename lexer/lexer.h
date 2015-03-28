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
  INCR,
  DECR,
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

enum Modifier {
  PUBLIC,
  PROTECTED,
  ABSTRACT,
  STATIC,
  FINAL,
  NATIVE,
  NUM_MODIFIERS
};

struct TokenTypeInfo {
  static TokenTypeInfo New(TokenType type, string repr, string value) {
    TokenTypeInfo t;
    t.type_ = type;
    t.repr_ = repr;
    t.value_ = value;
    t.mod_ = lexer::NUM_MODIFIERS;
    return t;
  }

  static const TokenTypeInfo& FromTokenType(TokenType type);

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
    ret.kind_ = Kind(ret.kind_ & ~SUPPORTED);
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

  TokenTypeInfo Skippable() const {
    TokenTypeInfo ret = *this;
    ret.kind_ = Kind(ret.kind_ | SKIPPABLE);
    return ret;
  }

  TokenTypeInfo MakeModifier(Modifier m) const {
    TokenTypeInfo ret = *this;
    ret.kind_ = Kind(ret.kind_ | MODIFIER);
    ret.mod_ = m;
    return ret;
  }

  TokenType Type() const { return type_; }
  bool IsSupported() const { return (kind_ & SUPPORTED) == SUPPORTED; }
  bool IsKeyword() const { return (kind_ & KEYWORD) == KEYWORD; }
  bool IsBinOp() const { return (kind_ & BIN_OP) == BIN_OP; }
  bool IsUnaryOp() const { return (kind_ & UNARY_OP) == UNARY_OP; }
  bool IsPrimitive() const { return (kind_ & PRIMITIVE) == PRIMITIVE; }
  bool IsLiteral() const { return (kind_ & LITERAL) == LITERAL; }
  bool IsSymbol() const { return (kind_ & SYMBOL) == SYMBOL; }
  bool IsSkippable() const { return (kind_ & SKIPPABLE) == SKIPPABLE; }
  bool IsModifier() const { return (kind_ & MODIFIER) == MODIFIER; }
  Modifier GetModifier() const {
    CHECK(IsModifier());
    return mod_;
  }

  int BinOpPrec() const {
    CHECK(IsBinOp());
    return precedence_;
  }

  const string& Value() const { return value_; }

  static const TokenTypeInfo kEntries[NUM_TOKEN_TYPES];

 private:
  // NOTE: do not replace this with "= default", it triggers some weird codegen
  // bug on linux.student.cs.
  TokenTypeInfo(const TokenTypeInfo& other)
      : type_(other.type_),
        kind_(other.kind_),
        precedence_(other.precedence_),
        mod_(other.mod_),
        repr_(other.repr_),
        value_(other.value_) {}

  enum Kind {
    NONE = 0,
    SUPPORTED = 1 << 0,
    KEYWORD = 1 << 1,
    BIN_OP = 1 << 2,
    UNARY_OP = 1 << 3,
    PRIMITIVE = 1 << 4,
    LITERAL = 1 << 5,
    SYMBOL = 1 << 6,
    SKIPPABLE = 1 << 7,
    MODIFIER = 1 << 8,
  };

  friend std::ostream& operator<<(std::ostream& out, const TokenTypeInfo& t);

  TokenTypeInfo() {}

  TokenType type_ = NUM_TOKEN_TYPES;
  Kind kind_ = SUPPORTED;
  int precedence_ = -1;  // Only valid if kind_ == BIN_OP.
  Modifier mod_ = NUM_MODIFIERS;
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

  const TokenTypeInfo& TypeInfo() const;

  TokenType type;
  base::PosRange pos;
};

void LexJoosFiles(const base::FileSet* fs, vector<vector<Token>>* tokens_out,
                  base::ErrorList* errors_out);

void StripSkippableTokens(const vector<Token>& tokens, vector<Token>* out);
void StripSkippableTokens(const vector<vector<Token>>& tokens,
                          vector<vector<Token>>* out);

void FindUnsupportedTokens(const vector<Token>& tokens,
                           base::ErrorList* errors);
void FindUnsupportedTokens(const vector<vector<Token>>& tokens,
                           base::ErrorList* errors);

bool IsBoolOp(TokenType op);
bool IsRelationalOp(TokenType op);
bool IsEqualityOp(TokenType op);
bool IsNumericOp(TokenType op);

jchar ConvertCharEscape(string s, u64 start, u64* next);
jstring ConvertStringEscapes(string s);

}  // namespace lexer

#endif
