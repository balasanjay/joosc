#include "lexer/lexer.h"
#include "lexer/lexer_internal.h"

using base::File;

namespace lexer {

string TokenTypeToString(TokenType t) {
  if (t >= NUM_TOKEN_TYPES || t < 0) {
    throw "invalid token type";
  }

  return internal::kTokenTypeToString[t];
}

namespace internal {

struct LexState {
  LexState(File* file, int fileid, vector<Token>* tokens, vector<Error>* errors)
      : file(file), fileid(fileid), tokens(tokens), errors(errors) {}

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

  void EmitError(Error err) { errors->push_back(err); }

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

  File* file;
  int fileid;

  // This is the range of the lexeme currently being constructed.
  int begin = 0;
  int end = 0;

  vector<Token>* tokens;
  vector<Error>* errors;

  StateFn stateFn;
};

bool IsWhitespace(u8 c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

bool IsNumeric(u8 c) { return '0' <= c && c <= '9'; }
bool IsOctal(u8 c) { return '0' <= c && c <= '7'; }

bool IsAlpha(u8 c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'); }

bool IsAlphaNumeric(u8 c) { return IsAlpha(c) || IsNumeric(c); }

bool IsStringEscapable(u8 c) {
  return c == 'b' | c == 't' | c == 'n' | c == 'f' | c == 'r' | c == '\'' |
         c == '"' | c == '\\' | IsOctal(c);
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
  } else if (IsAlpha(state->Peek())) {
    state->SetNextState(&Identifier);
    return;
  }

  // This should be run after checking for comment tokens.
  for (int i = 0; i < kNumSymbolLiterals; ++i) {
    const string& symbolString = kSymbolLiterals[i].first;

    if (state->HasPrefix(symbolString)) {
      state->Advance(symbolString.size());
      state->EmitToken(kSymbolLiterals[i].second);
      return;
    }
  }

  throw "Found unknown prefix.";
}

void Integer(LexState* state) {
  while (!state->IsAtEnd() && IsNumeric(state->Peek())) {
    // Reject multi-digit number that starts with 0.
    if (state->begin + 1 == state->end &&
        state->file->At(state->begin) == '0') {
      throw "Multi-digit integer starting with 0";
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
      // TODO: emit unclosed block comment error.
      throw "unimplemented";
      state->SetNextState(nullptr);
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
  if (state->IsAtEnd() || !IsAlpha(state->Peek())) {
    throw "Tried to lex identifier that starts with non-alpha character";
  }
  while (!state->IsAtEnd() && IsAlphaNumeric(state->Peek())) {
    state->Advance();
  }

  state->EmitToken(IDENTIFIER);
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

  // Advance past normal or escape char.
  if (state->Peek() == '\\') {
    if (!AdvanceEscapedChar(state)) {
      throw "Invalid escaped character in character literal.";
    }
  } else {
    state->Advance();
  }
  // Require another apostrophe.
  if (state->Peek() != '\'') {
    throw "Unclosed character literal. Expected ' at __.";
  }
  state->Advance();
  state->EmitToken(CHAR);
  state->SetNextState(&Start);
}

void String(LexState* state) {
  state->Advance();  // Advance past the quotation mark.

  while (true) {
    if (state->IsAtEnd()) {
      throw "Unclosed string at end of file.";
    }

    u8 next = state->Peek();
    state->Advance();

    if (next == '\n') {
      throw "Unclosed string at line end.";
    } else if (next == '"') {
      break;
    } else if (next == '\\') {
      if (!AdvanceEscapedChar(state)) {
        throw "Invalid escaped character in string literal.";
      }
    }
  }

  state->EmitToken(STRING);
  state->SetNextState(&Start);
}

}  // namespace internal

void LexJoosFile(base::File* file, int fileid, vector<Token>* tokens_out,
                 vector<Error>* errors_out) {
  // Remove anything with non-ANSI characters.
  for (int i = 0; i < file->Size(); i++) {
    if (file->At(i) > 127) {
      // TODO: more specific error.
      errors_out->push_back(Error(NON_ANSI_CHAR, PosRange(fileid, i, i + 1)));
      return;
    }
  }

  internal::LexState state(file, fileid, tokens_out, errors_out);
  state.SetNextState(&internal::Start);
  state.Run();
}

void LexJoosFiles(base::FileSet* fs, vector<vector<Token>>* tokens_out,
                  vector<Error>* errors_out) {
  tokens_out->clear();
  tokens_out->resize(fs->Size());

  for (int i = 0; i < fs->Size(); i++) {
    LexJoosFile(fs->Get(i), i, &(*tokens_out)[i], errors_out);
  }
}

}  // namespace lexer
