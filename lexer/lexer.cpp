#include "lexer/lexer.h"
#include "lexer/lexer_error.h"

using base::Error;
using base::ErrorList;
using base::File;
using base::FileSet;
using base::Pos;
using base::PosRange;

namespace lexer {

namespace {

#define NEW(repr, value) TokenTypeInfo::New(repr, #repr, value)

const TokenTypeInfo kTokenTypeInfo[NUM_TOKEN_TYPES] = {
  NEW(LINE_COMMENT, "LINE_COMMENT").Skippable(),
  NEW(BLOCK_COMMENT, "BLOCK_COMMENT").Skippable(),
  NEW(WHITESPACE, "WHITESPACE").Skippable(),
  NEW(LE, "<=").BinOp(7),
  NEW(GE, ">=").BinOp(7),
  NEW(EQ, "==").BinOp(6),
  NEW(NEQ, "!=").BinOp(6),
  NEW(AND, "&&").BinOp(2),
  NEW(OR, "||").BinOp(1),
  NEW(INCR, "++").UnaryOp().Unsupported(),
  NEW(DECR, "--").UnaryOp().Unsupported(),
  NEW(ADD, "+").BinOp(8),
  NEW(SUB, "-").BinOp(8).UnaryOp(),
  NEW(MUL, "*").BinOp(9),
  NEW(DIV, "/").BinOp(9),
  NEW(MOD, "%").BinOp(9),
  NEW(LT, "<").BinOp(7),
  NEW(GT, ">").BinOp(7),
  NEW(BAND, "&").BinOp(5),
  NEW(BOR, "|").BinOp(3),
  NEW(XOR, "^").BinOp(4),
  NEW(NOT, "!").UnaryOp(),
  NEW(ASSG, "=").BinOp(0),
  NEW(LPAREN, "(").Symbol(),
  NEW(RPAREN, ")").Symbol(),
  NEW(LBRACE, "{").Symbol(),
  NEW(RBRACE, "}").Symbol(),
  NEW(LBRACK, "[").Symbol(),
  NEW(RBRACK, "]").Symbol(),
  NEW(SEMI, ";").Symbol(),
  NEW(COMMA, ",").Symbol(),
  NEW(DOT, ".").Symbol(),
  NEW(INTEGER, "INTEGER").Literal(),
  NEW(IDENTIFIER, "IDENTIFIER"),
  NEW(CHAR, "CHAR").Literal(),
  NEW(STRING, "STRING").Literal(),
  // Keywords.
  NEW(K_ABSTRACT, "abstract").Keyword().MakeModifier(ABSTRACT),
  NEW(K_DEFAULT, "default").Keyword().Unsupported(),
  NEW(K_IF, "if").Keyword(),
  NEW(K_PRIVATE, "private").Keyword().Unsupported(),
  NEW(K_THIS, "this").Keyword(),
  NEW(K_BOOL, "boolean").Keyword().Primitive(),
  NEW(K_DO, "do").Keyword().Unsupported(),
  NEW(K_IMPLEMENTS, "implements").Keyword(),
  NEW(K_PROTECTED, "protected").Keyword().MakeModifier(PROTECTED),
  NEW(K_THROW, "throw").Keyword().Unsupported(),
  NEW(K_BREAK, "break").Keyword().Unsupported(),
  NEW(K_DOUBLE, "double").Keyword().Unsupported(),
  NEW(K_IMPORT, "import").Keyword(),
  NEW(K_PUBLIC, "public").Keyword().MakeModifier(PUBLIC),
  NEW(K_THROWS, "throws").Keyword().Unsupported(),
  NEW(K_BYTE, "byte").Keyword().Primitive(),
  NEW(K_ELSE, "else").Keyword(),
  NEW(K_INSTANCEOF, "instanceof").Keyword().BinOp(7),
  NEW(K_RETURN, "return").Keyword(),
  NEW(K_TRANSIENT, "transient").Keyword().Unsupported(),
  NEW(K_CASE, "case").Keyword().Unsupported(),
  NEW(K_EXTENDS, "extends").Keyword(),
  NEW(K_INT, "int").Keyword().Primitive(),
  NEW(K_SHORT, "short").Keyword().Primitive(),
  NEW(K_TRY, "try").Keyword().Unsupported(),
  NEW(K_CATCH, "catch").Keyword().Unsupported(),
  NEW(K_FINAL, "final").Keyword().MakeModifier(FINAL),
  NEW(K_INTERFACE, "interface").Keyword(),
  NEW(K_STATIC, "static").Keyword().MakeModifier(STATIC),
  NEW(K_VOID, "void").Keyword().Primitive(),
  NEW(K_CHAR, "char").Keyword().Primitive(),
  NEW(K_FINALLY, "finally").Keyword().Unsupported(),
  NEW(K_LONG, "long").Keyword().Unsupported(),
  NEW(K_STRICTFP, "strictfp").Keyword().Unsupported(),
  NEW(K_VOLATILE, "volatile").Keyword().Unsupported(),
  NEW(K_CLASS, "class").Keyword(),
  NEW(K_FLOAT, "float").Keyword(),
  NEW(K_NATIVE, "native").Keyword().MakeModifier(NATIVE),
  NEW(K_SUPER, "super").Keyword().Unsupported(),
  NEW(K_WHILE, "while").Keyword(),
  NEW(K_CONST, "const").Keyword().Unsupported(),
  NEW(K_FOR, "for").Keyword(),
  NEW(K_NEW, "new").Keyword(),
  NEW(K_SWITCH, "switch").Keyword().Unsupported(),
  NEW(K_CONTINUE, "continue").Keyword().Unsupported(),
  NEW(K_GOTO, "goto").Keyword().Unsupported(),
  NEW(K_PACKAGE, "package").Keyword(),
  NEW(K_SYNCHRONIZED, "synchronized").Keyword().Unsupported(),
  NEW(K_TRUE, "true").Keyword().Literal(),
  NEW(K_FALSE, "false").Keyword().Literal(),
  NEW(K_NULL, "null").Keyword().Literal(),
};

#undef NEW

} // namespace

std::ostream& operator<<(std::ostream& out, const TokenTypeInfo& t) {
  return out << t.repr_;
}

TokenTypeInfo TokenTypeInfo::FromTokenType(TokenType type) {
  assert(0 <= type && type < NUM_TOKEN_TYPES);
  return kTokenTypeInfo[type];
}

TokenTypeInfo Token::TypeInfo() const {
  return TokenTypeInfo::FromTokenType(type);
}

namespace internal {

struct LexState {
  LexState(FileSet* fs, File* file, int fileid, vector<Token>* tokens,
           base::ErrorList* errors)
      : fs(fs), file(file), fileid(fileid), tokens(tokens), errors(errors) {}

  typedef void (*StateFn)(LexState*);

  // Returns true if the cursor is at EOF.
  bool IsAtEnd() { return end >= file->Size(); }

  // Advance the cursor by n characters.
  void Advance(int n = 1) {
    if (IsAtEnd()) {
      throw "weird state";
    }

    end += n;
  }

  // Get the character at the cursor.
  u8 Peek() { return file->At(end); }

  // Returns true iff the file has a prefix s, starting at the current cursor.
  bool HasPrefix(string s) {
    if (size_t(file->Size() - end) < s.size()) {
      return false;
    }

    for (size_t i = 0; i < s.size(); i++) {
      if (file->At(end + i) != s.at(i)) {
        return false;
      }
    }
    return true;
  }

  // Emit a token starting at the end of the previous token (or 0 by default),
  // and ending at the cursor.
  void EmitToken(TokenType tok) {
    // TODO: implement some assert macro.
    if (begin >= end) {
      throw "weird state";
    }

    tokens->push_back(Token(tok, PosRange(fileid, begin, end)));
    begin = end;
  }

  void EmitFatal(Error* err) {
    errors->Append(err);
    SetNextState(nullptr);
  }

  void SetNextState(StateFn fn) { stateFn = fn; }

  void Run() {
    while (stateFn != nullptr) {
      StateFn prevStateFn = stateFn;
      int prevBegin = begin;
      int prevEnd = end;

      stateFn(this);

      if (stateFn == prevStateFn && begin == prevBegin && end == prevEnd) {
        throw "made no forward progress";
      }
    }
  }

  FileSet* fs;
  File* file;
  int fileid;

  // This is the range of the lexeme currently being constructed.
  int begin = 0;
  int end = 0;

  vector<Token>* tokens;
  base::ErrorList* errors;

  StateFn stateFn;
};

bool IsWhitespace(u8 c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

bool IsNumeric(u8 c) { return '0' <= c && c <= '9'; }
bool IsOctal(u8 c) { return '0' <= c && c <= '7'; }

bool IsAlpha(u8 c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }

bool IsAlphaNumeric(u8 c) { return IsAlpha(c) || IsNumeric(c); }

bool IsValidIdentifierStartChar(u8 c) {
  return IsAlpha(c) || c == '_' || c == '$';
}

bool IsValidIdentifierChar(u8 c) {
  return IsValidIdentifierStartChar(c) || IsNumeric(c);
}

bool IsStringEscapable(u8 c) {
  return c == 'b' | c == 't' | c == 'n' | c == 'f' | c == 'r' | c == '\'' |
         c == '"' | c == '\\' | IsOctal(c);
}

bool PosRangeStringMatches(const FileSet* fs, const PosRange& range,
                           const string& s) {
  if ((int)s.size() != range.end - range.begin) {
    return false;
  }
  File* f = fs->Get(range.fileid);
  for (u64 i = 0; i < s.size(); ++i) {
    if (s[i] != f->At(range.begin + i)) {
      return false;
    }
  }
  return true;
}

/**
 * Tries to match a string with all tokens/reserved words.
 * Returns the type of the keyword, or IDENTIFIER if no keywords were matched.
 */
TokenType MatchKeywords(const FileSet* fs, const PosRange& range) {
  for (int i = 0; i < NUM_TOKEN_TYPES; ++i) {
    TokenTypeInfo typeInfo = kTokenTypeInfo[i];
    if (!typeInfo.IsKeyword()) {
      continue;
    }

    if (PosRangeStringMatches(fs, range, typeInfo.Value())) {
      return typeInfo.Type();
    }
  }

  return IDENTIFIER;
}

void Start(LexState* state);
void Integer(LexState* state);
void Whitespace(LexState* state);
void BlockComment(LexState* state);
void LineComment(LexState* state);
void Identifier(LexState* state);
void Char(LexState* state);
void String(LexState* state);

void Start(LexState* state) {
  // Should have no characters in current range when in start state.
  if (state->begin != state->end) {
    throw "Partial lexeme when in start state.";
  }
  if (state->IsAtEnd()) {
    state->SetNextState(nullptr);
    return;
  }

  if (state->HasPrefix("//")) {
    state->SetNextState(&LineComment);
    return;
  } else if (state->HasPrefix("/*")) {
    state->SetNextState(&BlockComment);
    return;
  } else if (state->Peek() == '\'') {
    state->SetNextState(&Char);
    return;
  } else if (state->Peek() == '"') {
    state->SetNextState(&String);
    return;
  } else if (IsWhitespace(state->Peek())) {
    state->SetNextState(&Whitespace);
    return;
  } else if (IsNumeric(state->Peek())) {
    state->SetNextState(&Integer);
    return;
  } else if (IsValidIdentifierStartChar(state->Peek())) {
    state->SetNextState(&Identifier);
    return;
  }

  // This should be run after checking for comment tokens.
  for (int i = 0; i < NUM_TOKEN_TYPES; ++i) {
    TokenTypeInfo typeInfo = kTokenTypeInfo[i];
    if (!typeInfo.IsSymbol()) {
      continue;
    }

    const string& symbolString = typeInfo.Value();
    if (state->HasPrefix(symbolString)) {
      state->Advance(symbolString.size());
      state->EmitToken(typeInfo.Type());
      return;
    }
  }

  state->EmitFatal(
      new UnexpectedCharError(state->fs, Pos(state->fileid, state->begin)));
}

void Integer(LexState* state) {
  while (!state->IsAtEnd() && IsNumeric(state->Peek())) {
    // Reject multi-digit number that starts with 0.
    if (state->begin + 1 == state->end &&
        state->file->At(state->begin) == '0') {

      state->EmitFatal(new LeadingZeroInIntLitError(
          state->fs, Pos(state->fileid, state->begin)));
      return;
    }
    state->Advance();
  }

  state->EmitToken(INTEGER);
  state->SetNextState(&Start);
}

void Whitespace(LexState* state) {
  while (!state->IsAtEnd() && IsWhitespace(state->Peek())) {
    state->Advance();
  }

  state->EmitToken(WHITESPACE);
  state->SetNextState(&Start);
}

void LineComment(LexState* state) {
  state->Advance(2);  // Advance past the "//".

  while (!state->IsAtEnd()) {
    // TODO: handle windows newlines?
    u8 next = state->Peek();
    state->Advance();

    if (next == '\n') {
      break;
    }
  }

  state->EmitToken(LINE_COMMENT);
  state->SetNextState(&Start);
}

void BlockComment(LexState* state) {
  state->Advance(2);  // Advance past the "/*".

  bool prevStar = false;

  while (true) {
    if (state->IsAtEnd()) {
      state->EmitFatal(new UnclosedBlockCommentError(
          state->fs, PosRange(state->fileid, state->begin, state->begin + 2)));
      return;
    }

    u8 next = state->Peek();
    state->Advance();

    if (prevStar && next == '/') {
      break;
    }
    prevStar = (next == '*');
  }

  state->EmitToken(BLOCK_COMMENT);
  state->SetNextState(&Start);
}

void Identifier(LexState* state) {
  if (state->IsAtEnd() || !IsValidIdentifierStartChar(state->Peek())) {
    throw "Internal error: Tried to lex identifier at invalid start char.";
  }
  state->Advance();  // Advance past first char.

  while (!state->IsAtEnd() && IsValidIdentifierChar(state->Peek())) {
    state->Advance();
  }

  TokenType keywordOrIdentifier = MatchKeywords(
      state->fs, PosRange(state->fileid, state->begin, state->end));
  state->EmitToken(keywordOrIdentifier);
  state->SetNextState(&Start);
}

// Advances lexer past escaped character.
// Returns true if well-formed escaped character found at cursor, false
// otherwise.
bool AdvanceEscapedChar(LexState* state) {
  state->Advance();  // Advance past backslash.
  u8 first = state->Peek();
  if (!IsStringEscapable(first)) {
    return false;
  } else if (!IsOctal(first)) {
    state->Advance();
    return true;
  }

  const int maxOctalEscape = 377;
  const int maxOctalDigits = 3;
  int currentOctalEscape = 0;

  // Include up to maxOctalDigits digits.
  for (int i = 0; i < maxOctalDigits; ++i) {
    u8 next = state->Peek();
    if (!IsOctal(next)) {
      break;
    }
    currentOctalEscape = currentOctalEscape * 10 + (next - '0');
    if (currentOctalEscape > maxOctalEscape) {
      // Don't include next digit if it would make the value larger than
      // maxOctalEscape.
      break;
    }
    state->Advance();
  }
  return true;
}

void Char(LexState* state) {
  state->Advance();  // Advance past apostrophe.

  if (state->IsAtEnd()) {
    state->EmitFatal(new InvalidCharacterLitError(
        state->fs, PosRange(state->fileid, state->begin, state->end)));
    return;
  }

  // Advance past normal or escape char.
  u8 next = state->Peek();
  if (next == '\\') {
    if (!AdvanceEscapedChar(state)) {
      state->EmitFatal(new InvalidCharacterEscapeError(
          state->fs, PosRange(state->fileid, state->begin, state->end)));
      return;
    }
  } else if (next == '\'' || next == '\n') {
    state->EmitFatal(new InvalidCharacterLitError(
        state->fs, PosRange(state->fileid, state->begin, state->end)));
    return;
  } else {
    state->Advance();
  }

  // Require another apostrophe.
  if (state->IsAtEnd()) {
    state->EmitFatal(new InvalidCharacterLitError(
        state->fs, PosRange(state->fileid, state->begin, state->end)));
    return;
  } else if (state->Peek() != '\'') {
    state->EmitFatal(new InvalidCharacterLitError(
        state->fs, PosRange(state->fileid, state->begin, state->end)));
    return;
  }
  state->Advance();
  state->EmitToken(CHAR);
  state->SetNextState(&Start);
}

void String(LexState* state) {
  state->Advance();  // Advance past the quotation mark.

  while (true) {
    if (state->IsAtEnd()) {
      state->EmitFatal(new UnclosedStringLitError(
          state->fs, Pos(state->fileid, state->begin)));
      return;
    }

    u8 next = state->Peek();
    if (next == '\n') {
      state->EmitFatal(new UnclosedStringLitError(
          state->fs, Pos(state->fileid, state->begin)));
      return;
    } else if (next == '"') {
      state->Advance();
      break;
    } else if (next == '\\') {
      int startEscapePos = state->end;
      if (!AdvanceEscapedChar(state)) {
        state->EmitFatal(new InvalidCharacterEscapeError(
            state->fs, PosRange(state->fileid, startEscapePos, state->end)));
        return;
      }
    } else {
      // For any other character, just make it part of the string.
      state->Advance();
    }
  }

  state->EmitToken(STRING);
  state->SetNextState(&Start);
}

}  // namespace internal

void LexJoosFile(base::FileSet* fs, base::File* file, int fileid,
                 vector<Token>* tokens_out, base::ErrorList* errors_out) {
  // Remove anything with non-ANSI characters.
  for (int i = 0; i < file->Size(); i++) {
    u8 c = file->At(i);
    if (c > 127) {
      errors_out->Append(new NonAnsiCharError(fs, Pos(fileid, i)));
      return;
    }
  }

  internal::LexState state(fs, file, fileid, tokens_out, errors_out);
  state.SetNextState(&internal::Start);
  state.Run();
}

void LexJoosFiles(base::FileSet* fs, vector<vector<Token>>* tokens_out,
                  base::ErrorList* errors_out) {
  tokens_out->clear();
  tokens_out->resize(fs->Size());

  for (int i = 0; i < fs->Size(); i++) {
    LexJoosFile(fs, fs->Get(i), i, &(*tokens_out)[i], errors_out);
  }
}

void StripSkippableTokens(const vector<Token>& tokens, vector<Token>* out) {
  for (Token token : tokens) {
    if (token.TypeInfo().IsSkippable()) {
      continue;
    }
    out->push_back(token);
  }
}

void StripSkippableTokens(const vector<vector<Token>>& tokens, vector<vector<Token>>* out) {
  for (const auto& file_tokens : tokens) {
    uint i = out->size();
    out->resize(i + 1);
    StripSkippableTokens(file_tokens, &out->at(i));
  }
}

void FindUnsupportedTokens(const base::FileSet* fs, const vector<Token>& tokens, base::ErrorList* errors) {
  for (const auto& tok: tokens) {
    if (!tok.TypeInfo().IsSupported()) {
      errors->Append(new UnsupportedTokenError(fs, tok.pos));
    }
  }
}
void FindUnsupportedTokens(const base::FileSet* fs, const vector<vector<Token>>& tokens, base::ErrorList* errors) {
  for (const auto& file_tokens : tokens) {
    FindUnsupportedTokens(fs, file_tokens, errors);
  }
}

}  // namespace lexer
