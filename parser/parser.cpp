#include "base/unique_ptr_vector.h"
#include "lexer/lexer.h"
#include "parser/ast.h"

using base::Error;
using std::move;
using base::ErrorList;
using base::File;
using base::FileSet;
using base::Pos;
using base::UniquePtrVector;
using lexer::ADD;
using lexer::ASSG;
using lexer::DOT;
using lexer::IDENTIFIER;
using lexer::INTEGER;
using lexer::K_THIS;
using lexer::LBRACK;
using lexer::LPAREN;
using lexer::MUL;
using lexer::RBRACK;
using lexer::RPAREN;
using lexer::Token;
using lexer::TokenType;
using std::cerr;
using std::stringstream;

struct repstr {
  repstr(int n, string str) : n_(n), str_(str) {}
  friend std::ostream& operator<<(std::ostream&, const repstr& r);
 private:
  int n_;
  string str_;
};

std::ostream& operator<<(std::ostream& os, const repstr& r) {
  for (int i = 0; i < r.n_; ++i) {
    os << r.str_;
  }
  return os;
}

struct ScopedPrint {
  ScopedPrint(const string& construct, const string& destruct) : construct_(construct), destruct_(destruct) {
    cerr << repstr(level_, "|  ") << construct_ << '\n';
    ++level_;
  }
  ~ScopedPrint() {
    --level_;
    cerr << repstr(level_, "|  ") << destruct_ << '\n';
  }



private:
  string construct_;
  string destruct_;
  static int level_;
};

int ScopedPrint::level_ = 0;

#define RETURN_IF_ERR(check) {\
  if (!(check).IsSuccess()) { \
    return (check); \
  } \
}

#define RETURN_IF_GOOD(state, value, out) {\
  State2 _hope_you_didnt_declare_this = (state); \
  if ((_hope_you_didnt_declare_this)) {\
    return (_hope_you_didnt_declare_this).Success2((value), (out)); \
  } \
}

#define SHORT_CIRCUIT \
  ScopedPrint _scoped_print(string("Entering ") + __FUNCTION__, string("Leaving ") + __FUNCTION__); \
{\
  if (!(*this)) { \
    return *this; \
  }\
}

namespace parser {

namespace {

// Predicates for ParseTokenIf.
struct ExactType {
  ExactType(TokenType type) : type_(type) {}
  bool operator()(const Token& t) { return t.type == type_; }
private:
  TokenType type_;
};
struct IsBinOp {
  bool operator()(const Token& t) { return t.TypeInfo().IsBinOp(); }
};
struct IsUnaryOp {
  bool operator()(const Token& t) { return t.TypeInfo().IsUnaryOp(); }
};
struct IsLiteral {
  bool operator()(const Token& t) { return t.TypeInfo().IsLiteral(); }
};
struct IsPrimitive {
  bool operator()(const Token& t) { return t.TypeInfo().IsPrimitive(); }
};

template <typename T>
class Result2 final {
public:
  Result2() = default;
  Result2(Result2&& other) = default;
  Result2& operator=(Result2&& other) = default;

  explicit operator bool() const {
    return IsSuccess();
  }

  bool IsSuccess() const {
    return !errors_.IsFatal();
  }

  T* Get() const {
    if (!IsSuccess()) {
      throw "Get() from non-successful result.";
    }
    return data_.get();
  }

  T* Release() {
    if (!IsSuccess()) {
      throw "Release() from non-successful result.";
    }
    return data_.release();
  }

  void ReleaseErrors(ErrorList* out) {
    vector<Error*> errors;
    errors_.Release(&errors);
    for (auto err : errors) {
      out->Append(err);
    }
  }

  const ErrorList& Errors() const {
    return errors_;
  }

private:
  DISALLOW_COPY_AND_ASSIGN(Result2);

  Result2(T* data) : success_(true), data_(data) {}
  Result2(Error* err) {
    errors_.Append(err);
  }
  Result2(ErrorList&& errors) : success_(false), errors_(std::forward<ErrorList>(errors)) {}

  template <typename U>
  friend Result2<U> Success(U* t);

  template <typename U>
  friend Result2<U> Failure(Error* e);

  template <typename U>
  friend Result2<U> Failure(ErrorList&&);

  template <typename T1, typename T2>
  friend Result2<T2> ConvertError(Result2<T1>&&);

  bool success_ = false;
  unique_ptr<T> data_;
  ErrorList errors_;
};

template <typename T>
Result2<T> Success(T* t) {
  return Result2<T>(t);
}
template <typename T>
Result2<T> Failure(Error* e) {
  return Result2<T>(e);
}
template <typename T>
Result2<T> Failure(ErrorList&& e) {
  return Result2<T>(std::forward<ErrorList>(e));
}

template <typename T>
void FirstOf(ErrorList* out, T* result) {
  result->ReleaseErrors(out);
}

template <typename T, typename... Rest>
void FirstOf(ErrorList* out, T* first, Rest... rest) {
  if (first->Errors().Size() == 0) {
    return FirstOf(out, rest...);
  }

  first->ReleaseErrors(out);
}

template <typename T, typename U>
Result2<U> ConvertError(Result2<T>&& r) {
  return Result2<U>(move(r.errors_));
}

struct State2 {
  State2(const FileSet* fs, const File* file, const vector<Token>* tokens, int index = 0, bool failed = false) : fs_(fs), file_(file), tokens_(tokens), index_(index), failed_(failed) {}

  State2 ParseTokenIf(std::function<bool(Token)> pred, Result2<Token>* out) const;

  // Type-related parsers.
  State2 ParseQualifiedName(Result2<QualifiedName>* out) const;
  State2 ParsePrimitiveType(Result2<Type>* out) const;
  State2 ParseSingleType(Result2<Type>* out) const;
  State2 ParseType(Result2<Type>* out) const;

  // Expression parsers.
  State2 ParseExpression(Result2<Expr>*) const;
  State2 ParseUnaryExpression(Result2<Expr>*) const;
  State2 ParseCastExpression(Result2<Expr>* out) const;
  State2 ParsePrimary(Result2<Expr>* out) const;
  State2 ParsePrimaryBase(Result2<Expr>* out) const;
  State2 ParsePrimaryEnd(Expr* base, Result2<Expr>* out) const;
  State2 ParsePrimaryEndNoArrayAccess(Expr* base, Result2<Expr>* out) const;

  bool IsAtEnd() const {
    return failed_ || (uint)index_ >= tokens_->size();
  }

  bool Failed() const {
    return failed_;
  }

 private:
  explicit operator bool() const {
    return !failed_;
  }

  Error* MakeUnexpectedTokenError(Token token) const;
  Error* MakeUnexpectedEOFError() const;

  Token GetNext() const {
    assert(!IsAtEnd());
    return tokens_->at(index_);
  }

  State2 Advance(int i = 1) const {
    return State2(fs_, file_, tokens_, index_ + i, failed_);
  }

  template <typename T>
  State2 Fail(Error* error, Result2<T>* out) const {
    *out = Failure<T>(error);
    return Fail();
  }

  template <typename T>
  State2 Fail(ErrorList&& errors, Result2<T>* out) const {
    *out = Failure<T>(std::forward<ErrorList>(errors));
    return Fail();
  }

  State2 Fail() const {
    return State2(fs_, file_, tokens_, index_, /* failed */ true);
  }

  template <typename T, typename U>
  State2 Success2(T* t, Result2<U>* out) const {
    *out = Success<U>(t);
    return *this;
  }

  const FileSet* Fs() const { return fs_; }
  const File* GetFile() const { return file_; }

  const FileSet* fs_ = nullptr;
  const File* file_ = nullptr;
  const vector<Token>* tokens_ = nullptr;
  int index_ = -1;
  bool failed_ = false;
};

} // namespace

Expr* FixPrecedence(UniquePtrVector<Expr>&& owned_exprs, const vector<Token>& ops) {
  vector<Expr*> outstack;
  vector<Token> opstack;

  vector<Expr*> exprs;
  owned_exprs.Release(&exprs);

  assert(exprs.size() == ops.size() + 1);

  uint i = 0;
  while (i < exprs.size() + ops.size() || opstack.size() > 0) {
    if (i < exprs.size() + ops.size()) {
      if (i % 2 == 0) {
        // Expr off input.
        Expr* e = exprs.at(i/2);
        outstack.push_back(e);
        i++;
        continue;
      }

      // Op off input.
      assert(i % 2 == 1);
      Token op = ops.at(i/2);

      if (opstack.empty() ||
          (op.type == ASSG && op.TypeInfo().BinOpPrec() >= opstack.rbegin()->TypeInfo().BinOpPrec()) ||
          (op.type != ASSG && op.TypeInfo().BinOpPrec() > opstack.rbegin()->TypeInfo().BinOpPrec())) {
        opstack.push_back(op);
        i++;
        continue;
      }
    }

    assert(outstack.size() >= 2);

    Expr* rhs = *outstack.rbegin();
    Expr* lhs = *(outstack.rbegin() + 1);
    Token nextop = *opstack.rbegin();

    outstack.pop_back();
    outstack.pop_back();
    opstack.pop_back();

    outstack.push_back(new BinExpr(lhs, nextop, rhs));
  }

  assert(outstack.size() == 1);
  assert(opstack.size() == 0);
  return outstack.at(0);
}

string TokenString(const File* file, Token token) {
  stringstream s;
  for (int i = token.pos.begin; i < token.pos.end; ++i) {
    s << file->At(i);
  }
  return s.str();
}

QualifiedName* MakeQualifiedName(const File *file, const vector<Token>& tokens) {
  assert(tokens.size() > 0);
  assert((tokens.size() - 1) % 2 == 0);

  stringstream fullname;
  vector<string> parts;

  for (uint i = 0; i < tokens.size(); ++i) {
    string part = TokenString(file, tokens.at(i));
    fullname << part;
    if ((i % 2) == 0) {
      parts.push_back(part);
    }
  }

  return new QualifiedName(tokens, parts, fullname.str());
}

Error* State2::MakeUnexpectedTokenError(Token token) const {
  // TODO: say what you expected instead.
  return MakeSimplePosRangeError(fs_, Pos(token.pos.fileid, token.pos.begin), "UnexpectedTokenError", "Unexpected token.");
}

Error* State2::MakeUnexpectedEOFError() const {
  // TODO: say what you expected instead.
  // TODO: this will crash on an empty file.
  return MakeSimplePosRangeError(
      fs_,
      Pos(tokens_->at(0).pos.fileid, file_->Size() - 1),
      "UnexpectedEOFError",
      "Unexpected end-of-file."
  );
}

State2 State2::ParseTokenIf(std::function<bool(Token)> pred, Result2<Token> *out) const {
  if (IsAtEnd()) {
    return Fail(MakeUnexpectedEOFError(), out);
  }

  if (!pred(GetNext())) {
    return Fail(MakeUnexpectedTokenError(GetNext()), out);
  }
  return Advance().Success2(new Token(GetNext()), out);
}

State2 State2::ParseQualifiedName(Result2<QualifiedName>* out) const {
  // QualifiedName:
  //   Identifier {"." Identifier}
  SHORT_CIRCUIT;

  vector<Token> tokens;

  Result2<Token> ident;
  State2 cur = ParseTokenIf(ExactType(IDENTIFIER), &ident);
  if (!ident) {
    *out = ConvertError<Token, QualifiedName>(move(ident));
    return Fail();
  }
  tokens.push_back(*ident.Get());

  while (true) {
    Result2<Token> dot;
    Result2<Token> nextIdent;
    State2 next = cur
      .ParseTokenIf(ExactType(DOT), &dot)
      .ParseTokenIf(ExactType(IDENTIFIER), &nextIdent);
    if (!dot || !nextIdent) {
      return cur.Success2(MakeQualifiedName(GetFile(), tokens), out);
    }

    tokens.push_back(*dot.Get());
    tokens.push_back(*nextIdent.Get());
    cur = next;
  }
}

State2 State2::ParsePrimitiveType(Result2<Type>* out) const {
  // PrimitiveType:
  //   "byte"
  //   "short"
  //   "int"
  //   "char"
  //   "boolean"
  SHORT_CIRCUIT;

  Result2<Token> primitive;
  State2 after = ParseTokenIf(IsPrimitive(), &primitive);
  if (primitive) {
    *out = Success<Type>(new PrimitiveType(*primitive.Get()));
    return after;
  }

  // TODO: make an error here.
  return Fail(nullptr, out);
}

State2 State2::ParseSingleType(Result2<Type>* out) const {
  // SingleType:
  //   PrimitiveType
  //   QualifiedName
  SHORT_CIRCUIT;

  {
    Result2<Type> primitive;
    State2 after = ParsePrimitiveType(&primitive);
    if (after) {
      *out = move(primitive);
      return after;
    }
  }

  {
    Result2<QualifiedName> reference;
    State2 after = ParseQualifiedName(&reference);
    RETURN_IF_GOOD(after, new ReferenceType(reference.Release()), out);
  }

  // TODO: make an error here.
  return Fail(nullptr, out);
}

State2 State2::ParseType(Result2<Type>* out) const {
  // Type:
  //   SingleType ["[" "]"]
  SHORT_CIRCUIT;

  Result2<Type> single;
  State2 afterSingle = ParseSingleType(&single);
  if (!afterSingle) {
    *out = move(single);
    return afterSingle;
  }

  Result2<Token> lbrack;
  Result2<Token> rbrack;

  State2 afterArray = afterSingle
    .ParseTokenIf(ExactType(LBRACK), &lbrack)
    .ParseTokenIf(ExactType(RBRACK), &rbrack);
  RETURN_IF_GOOD(afterArray, new ArrayType(single.Release()), out);

  // If we didn't find brackets, then just use the initial expression on its own.
  *out = move(single);
  return afterSingle;
}


// Expression parsers.

State2 State2::ParseExpression(Result2<Expr>* out) const {
  // TODO: Regrammarize.
  SHORT_CIRCUIT;

  UniquePtrVector<Expr> exprs;
  vector<Token> operators;

  Result2<Expr> expr;
  State2 cur = ParseUnaryExpression(&expr);
  if (!cur) {
    *out = move(expr);
    return cur;
  }

  exprs.Append(expr.Release());

  while (true) {
    Result2<Token> binOp;
    Result2<Expr> nextExpr;

    State2 next = cur.ParseTokenIf(IsBinOp(), &binOp).ParseUnaryExpression(&nextExpr);
    if (!next) {
      return cur.Success2(FixPrecedence(move(exprs), operators), out);
    }

    operators.push_back(*binOp.Get());
    exprs.Append(nextExpr.Release());
    cur = next;
  }
}

State2 State2::ParseUnaryExpression(Result2<Expr>* out) const {
  // UnaryExpression:
  //   "-" UnaryExpression
  //   "!" UnaryExpression
  //   CastExpression
  //   Primary
  SHORT_CIRCUIT;

  // Prevent infinite recursion on first production.
  if (IsAtEnd()) {
    return Fail(MakeUnexpectedEOFError(), out);
  }

  {
    Result2<Token> unaryOp;
    Result2<Expr> expr;
    State2 after = ParseTokenIf(IsUnaryOp(), &unaryOp).ParseUnaryExpression(&expr);
    RETURN_IF_GOOD(after, new UnaryExpr(*unaryOp.Get(), expr.Release()), out);
  }

  {
    Result2<Expr> expr;
    State2 after = ParseCastExpression(&expr);
    RETURN_IF_GOOD(after, expr.Release(), out);
  }

  return ParsePrimary(out);
}

State2 State2::ParseCastExpression(Result2<Expr>* out) const {
  // CastExpression:
  //   "(" Type ")" UnaryExpression
  SHORT_CIRCUIT;

  Result2<Token> lparen;
  Result2<Type> type;
  Result2<Token> rparen;
  Result2<Expr> expr;

  State2 after = (*this)
    .ParseTokenIf(ExactType(LPAREN), &lparen)
    .ParseType(&type)
    .ParseTokenIf(ExactType(RPAREN), &rparen)
    .ParseUnaryExpression(&expr);
  RETURN_IF_GOOD(after, new CastExpr(type.Release(), expr.Release()), out);

  // Collect the first error, and use that.
  ErrorList errors;
  FirstOf(&errors, &lparen, &type, &rparen, &expr);
  return Fail(move(errors), out);
}

State2 State2::ParsePrimary(Result2<Expr>* out) const {
  // Primary:
  //   "new" SingleType NewEnd
  //   PrimaryBase [ PrimaryEnd ]
  SHORT_CIRCUIT;

  // TODO: "new" SingleType NewEnd

  Result2<Expr> base;
  State2 afterBase = ParsePrimaryBase(&base);
  if (!afterBase) {
    *out = move(base);
    return afterBase;
  }

  // We retain ownership IF ParsePrimaryEnd fails. If it succeeds, the
  // expectation is that it will take ownership.
  Expr* baseExpr = base.Release();
  Result2<Expr> baseWithEnds;
  State2 afterEnds = afterBase.ParsePrimaryEnd(baseExpr, &baseWithEnds);
  RETURN_IF_GOOD(afterEnds, baseWithEnds.Release(), out);


  // If we couldn't parse the PrimaryEnd, then just use base; it's optional.
  return afterBase.Success2(baseExpr, out);
}

State2 State2::ParsePrimaryBase(Result2<Expr>* out) const {
  // PrimaryBase:
  //   Literal
  //   "this"
  //   "(" Expression ")"
  //   QualifiedName
  SHORT_CIRCUIT;

  if (IsAtEnd()) {
    return Fail(MakeUnexpectedEOFError(), out);
  }

  {
    Result2<Token> litExpr;
    State2 after = ParseTokenIf(IsLiteral(), &litExpr);
    RETURN_IF_GOOD(after, new LitExpr(*litExpr.Get()), out);
  }

  {
    Result2<Token> thisExpr;
    State2 after = ParseTokenIf(ExactType(K_THIS), &thisExpr);
    RETURN_IF_GOOD(after, new ThisExpr(), out);
  }

  {
    Result2<Token> lparen;
    Result2<Expr> expr;
    Result2<Token> rparen;

    State2 after = (*this)
      .ParseTokenIf(ExactType(LPAREN), &lparen)
      .ParseExpression(&expr)
      .ParseTokenIf(ExactType(RPAREN), &rparen);
    RETURN_IF_GOOD(after, expr.Release(), out);
  }

  {
    Result2<QualifiedName> name;
    State2 after = ParseQualifiedName(&name);
    RETURN_IF_GOOD(after, new NameExpr(name.Release()), out);
  }

  return Fail(MakeUnexpectedTokenError(GetNext()), out);
}

State2 State2::ParsePrimaryEnd(Expr* base, Result2<Expr>* out) const {
  // PrimaryEnd:
  //   "[" Expression "]" [ PrimaryEndNoArrayAccess ]
  //   PrimaryEndNoArrayAccess
  SHORT_CIRCUIT;

  {
    Result2<Token> lbrack;
    Result2<Expr> expr;
    Result2<Token> rbrack;

    State2 after = (*this)
      .ParseTokenIf(ExactType(LBRACK), &lbrack)
      .ParseExpression(&expr)
      .ParseTokenIf(ExactType(RBRACK), &rbrack);
    RETURN_IF_GOOD(after, new ArrayIndexExpr(base, expr.Release()), out);
  }

  return ParsePrimaryEndNoArrayAccess(base, out);
}

State2 State2::ParsePrimaryEndNoArrayAccess(Expr* base, Result2<Expr>* out) const {
  // PrimaryEndNoArrayAccess:
  //   "." Identifier [ PrimaryEnd ]
  //   "(" [ArgumentList] ")" [ PrimaryEnd ]
  SHORT_CIRCUIT;

  {
    Result2<Token> dot;
    Result2<Token> ident;
    State2 after = (*this)
      .ParseTokenIf(ExactType(DOT), &dot)
      .ParseTokenIf(ExactType(IDENTIFIER), &ident);

    if (after) {
      Expr* deref = new FieldDerefExpr(base, TokenString(GetFile(), *ident.Get()), *ident.Get());
      Result2<Expr> nested;
      State2 afterEnd = after.ParsePrimaryEnd(deref, &nested);
      RETURN_IF_GOOD(afterEnd, nested.Release(), out);

      return after.Success2(deref, out);
    }
  }

  // TODO: "(" [ArgumentList] ")" [ PrimaryEnd ]

  return Fail(nullptr, out);
}

void Parse(const FileSet* fs, const File* file, const vector<Token>* tokens) {
  State2 state(fs, file, tokens, 0);
  Result2<Expr> result;
  state.ParseExpression(&result);
  if (result.IsSuccess()) {
    result.Get()->PrintTo(&std::cout);
    std::cout << '\n';
  } else {
    result.Errors().PrintTo(&std::cout, base::OutputOptions::kUserOutput);
  }
}


// TODO: in for-loop initializers, for-loop incrementors, and top-level
// statements, we must ensure that they are either assignment, method
// invocation, or class creation, not other types of expressions (like boolean
// ops).
//
// TODO: Weed out statements of the form "new PrimitiveType([ArgumentList])".

} // namespace parser
