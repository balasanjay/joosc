#include "lexer/lexer.h"

using base::File;

namespace lexer {

string tokenTypeToString[NUM_TOKEN_TYPES] = {
    "LINE_COMMENT", "BLOCK_COMMENT", "WHITESPACE", "IF", "WHILE"};

const int NUM_SYMBOL_LITERALS = 26;
pair<string, TokenType> symbolLiterals[NUM_SYMBOL_LITERALS] = {
  make_pair("<=", LE),
  make_pair(">=", GE),
  make_pair("==", EQ),
  make_pair("!=", NEQ),
  make_pair("&&", AND),
  make_pair("||", OR),
  make_pair("+", ADD),
  make_pair("-", SUB),
  make_pair("*", MUL),
  make_pair("/", DIV),
  make_pair("%", MOD),
  make_pair("<", LT),
  make_pair(">", GT),
  make_pair("&", BAND),
  make_pair("|", BOR),
  make_pair("!", NOT),
  make_pair("=", ASSG),
  make_pair("(", LPAREN),
  make_pair(")", RPAREN),
  make_pair("{", LBRACE),
  make_pair("}", RBRACE),
  make_pair("[", LBRACK),
  make_pair("]", RBRACK),
  make_pair(";", SEMI),
  make_pair(",", COMMA),
  make_pair(".", DOT)
};

string TokenTypeToString(TokenType t) {
  if (t >= NUM_TOKEN_TYPES || t < 0) {
    throw "invalid token type";
  }

  return tokenTypeToString[t];
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

void Start(LexState* state);
void Whitespace(LexState* state);
void BlockComment(LexState* state);
void LineComment(LexState* state);

void Start(LexState* state) {
  if (state->IsAtEnd()) {
    if (state->begin != state->end) {
      throw "unclosed token at eof";
    }
    state->SetNextState(nullptr);
    return;
  }

  if (state->HasPrefix("//")) {
    state->SetNextState(&LineComment);
    return;
  } else if (state->HasPrefix("/*")) {
    state->SetNextState(&BlockComment);
    return;
  } else if (IsWhitespace(state->Peek())) {
    state->SetNextState(&Whitespace);
    return;
  }

  // This should be run after checking for comment tokens.
  for (int i = 0; i < NUM_SYMBOL_LITERALS; ++i) {
    const string& symbolString = symbolLiterals[i].first;

    if (state->HasPrefix(symbolString)) {
      state->Advance(symbolString.size());
      state->EmitToken(symbolLiterals[i].second);
      return;
    }
  }

  // TODO: do the rest and emit unknown prefix error.
  state->SetNextState(nullptr);
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
