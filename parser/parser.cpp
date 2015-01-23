#include "base/unique_ptr_vector.h"
#include "lexer/lexer.h"
#include "parser/ast.h"

using base::Error;
using base::ErrorList;
using base::File;
using base::FileSet;
using base::Pos;
using base::UniquePtrVector;
using lexer::ADD;
using lexer::ASSG;
using lexer::COMMA;
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
using std::move;
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

#define RETURN_IF_GOOD(parser, value, out) {\
  Parser _hope_you_didnt_declare_this = (parser); \
  if ((_hope_you_didnt_declare_this)) {\
    return (_hope_you_didnt_declare_this).Success((value), (out)); \
  } \
}

#define SHORT_CIRCUIT {\
  if (!(*this)) { \
    return *this; \
  }\
};\
ScopedPrint _scoped_print(string("Entering ") + __FUNCTION__, string("Leaving ") + __FUNCTION__)

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
class Result final {
public:
  Result() = default;
  Result(Result&& other) = default;
  Result& operator=(Result&& other) = default;

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
  DISALLOW_COPY_AND_ASSIGN(Result);

  Result(T* data) : success_(true), data_(data) {}
  Result(Error* err) {
    errors_.Append(err);
  }
  Result(ErrorList&& errors) : success_(false), errors_(std::forward<ErrorList>(errors)) {}

  template <typename U>
  friend Result<U> MakeSuccess(U* t);

  template <typename U>
  friend Result<U> Failure(Error* e);

  template <typename U>
  friend Result<U> Failure(ErrorList&&);

  template <typename T1, typename T2>
  friend Result<T2> ConvertError(Result<T1>&&);

  bool success_ = false;
  unique_ptr<T> data_;
  ErrorList errors_;
};

template <typename T>
Result<T> MakeSuccess(T* t) {
  return Result<T>(t);
}
template <typename T>
Result<T> Failure(Error* e) {
  return Result<T>(e);
}
template <typename T>
Result<T> Failure(ErrorList&& e) {
  return Result<T>(std::forward<ErrorList>(e));
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
Result<U> ConvertError(Result<T>&& r) {
  return Result<U>(move(r.errors_));
}

struct Parser {
  Parser(const FileSet* fs, const File* file, const vector<Token>* tokens, int index = 0, bool failed = false) : fs_(fs), file_(file), tokens_(tokens), index_(index), failed_(failed) {}

  Parser ParseTokenIf(std::function<bool(Token)> pred, Result<Token>* out) const;

  // Type-related parsers.
  Parser ParseQualifiedName(Result<QualifiedName>* out) const;
  Parser ParsePrimitiveType(Result<Type>* out) const;
  Parser ParseSingleType(Result<Type>* out) const;
  Parser ParseType(Result<Type>* out) const;

  // Expression parsers.
  Parser ParseExpression(Result<Expr>*) const;
  Parser ParseUnaryExpression(Result<Expr>*) const;
  Parser ParseCastExpression(Result<Expr>* out) const;
  Parser ParsePrimary(Result<Expr>* out) const;
  Parser ParsePrimaryBase(Result<Expr>* out) const;
  Parser ParsePrimaryEnd(Expr* base, Result<Expr>* out) const;
  Parser ParsePrimaryEndNoArrayAccess(Expr* base, Result<Expr>* out) const;

  // Other parsers.
  Parser ParseArgumentList(Result<ArgumentList>*) const;

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

  Parser Advance(int i = 1) const {
    return Parser(fs_, file_, tokens_, index_ + i, failed_);
  }

  template <typename T>
  Parser Fail(Error* error, Result<T>* out) const {
    *out = Failure<T>(error);
    return Fail();
  }

  template <typename T>
  Parser Fail(ErrorList&& errors, Result<T>* out) const {
    *out = Failure<T>(std::forward<ErrorList>(errors));
    return Fail();
  }

  Parser Fail() const {
    return Parser(fs_, file_, tokens_, index_, /* failed */ true);
  }

  template <typename T, typename U>
  Parser Success(T* t, Result<U>* out) const {
    *out = MakeSuccess<U>(t);
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

Error* Parser::MakeUnexpectedTokenError(Token token) const {
  // TODO: say what you expected instead.
  return MakeSimplePosRangeError(fs_, Pos(token.pos.fileid, token.pos.begin), "UnexpectedTokenError", "Unexpected token.");
}

Error* Parser::MakeUnexpectedEOFError() const {
  // TODO: say what you expected instead.
  // TODO: this will crash on an empty file.
  return MakeSimplePosRangeError(
      fs_,
      Pos(tokens_->at(0).pos.fileid, file_->Size() - 1),
      "UnexpectedEOFError",
      "Unexpected end-of-file."
  );
}

Parser Parser::ParseTokenIf(std::function<bool(Token)> pred, Result<Token> *out) const {
  if (IsAtEnd()) {
    return Fail(MakeUnexpectedEOFError(), out);
  }

  if (!pred(GetNext())) {
    return Fail(MakeUnexpectedTokenError(GetNext()), out);
  }
  return Advance().Success(new Token(GetNext()), out);
}

Parser Parser::ParseQualifiedName(Result<QualifiedName>* out) const {
  // QualifiedName:
  //   Identifier {"." Identifier}
  SHORT_CIRCUIT;

  vector<Token> tokens;

  Result<Token> ident;
  Parser cur = ParseTokenIf(ExactType(IDENTIFIER), &ident);
  if (!ident) {
    *out = ConvertError<Token, QualifiedName>(move(ident));
    return Fail();
  }
  tokens.push_back(*ident.Get());

  while (true) {
    Result<Token> dot;
    Result<Token> nextIdent;
    Parser next = cur
      .ParseTokenIf(ExactType(DOT), &dot)
      .ParseTokenIf(ExactType(IDENTIFIER), &nextIdent);
    if (!dot || !nextIdent) {
      return cur.Success(MakeQualifiedName(GetFile(), tokens), out);
    }

    tokens.push_back(*dot.Get());
    tokens.push_back(*nextIdent.Get());
    cur = next;
  }
}

Parser Parser::ParsePrimitiveType(Result<Type>* out) const {
  // PrimitiveType:
  //   "byte"
  //   "short"
  //   "int"
  //   "char"
  //   "boolean"
  SHORT_CIRCUIT;

  Result<Token> primitive;
  Parser after = ParseTokenIf(IsPrimitive(), &primitive);
  RETURN_IF_GOOD(after, new PrimitiveType(*primitive.Get()), out);

  // TODO: make an error here.
  return Fail(nullptr, out);
}

Parser Parser::ParseSingleType(Result<Type>* out) const {
  // SingleType:
  //   PrimitiveType
  //   QualifiedName
  SHORT_CIRCUIT;

  {
    Result<Type> primitive;
    Parser after = ParsePrimitiveType(&primitive);
    if (after) {
      *out = move(primitive);
      return after;
    }
  }

  {
    Result<QualifiedName> reference;
    Parser after = ParseQualifiedName(&reference);
    RETURN_IF_GOOD(after, new ReferenceType(reference.Release()), out);
  }

  // TODO: make an error here.
  return Fail(nullptr, out);
}

Parser Parser::ParseType(Result<Type>* out) const {
  // Type:
  //   SingleType ["[" "]"]
  SHORT_CIRCUIT;

  Result<Type> single;
  Parser afterSingle = ParseSingleType(&single);
  if (!afterSingle) {
    *out = move(single);
    return afterSingle;
  }

  Result<Token> lbrack;
  Result<Token> rbrack;

  Parser afterArray = afterSingle
    .ParseTokenIf(ExactType(LBRACK), &lbrack)
    .ParseTokenIf(ExactType(RBRACK), &rbrack);
  RETURN_IF_GOOD(afterArray, new ArrayType(single.Release()), out);

  // If we didn't find brackets, then just use the initial expression on its own.
  *out = move(single);
  return afterSingle;
}


// Expression parsers.

Parser Parser::ParseExpression(Result<Expr>* out) const {
  // TODO: Regrammarize.
  SHORT_CIRCUIT;

  UniquePtrVector<Expr> exprs;
  vector<Token> operators;

  Result<Expr> expr;
  Parser cur = ParseUnaryExpression(&expr);
  if (!cur) {
    *out = move(expr);
    return cur;
  }

  exprs.Append(expr.Release());

  while (true) {
    Result<Token> binOp;
    Result<Expr> nextExpr;

    Parser next = cur.ParseTokenIf(IsBinOp(), &binOp).ParseUnaryExpression(&nextExpr);
    if (!next) {
      return cur.Success(FixPrecedence(move(exprs), operators), out);
    }

    operators.push_back(*binOp.Get());
    exprs.Append(nextExpr.Release());
    cur = next;
  }
}

Parser Parser::ParseUnaryExpression(Result<Expr>* out) const {
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
    Result<Token> unaryOp;
    Result<Expr> expr;
    Parser after = ParseTokenIf(IsUnaryOp(), &unaryOp).ParseUnaryExpression(&expr);
    RETURN_IF_GOOD(after, new UnaryExpr(*unaryOp.Get(), expr.Release()), out);
  }

  {
    Result<Expr> expr;
    Parser after = ParseCastExpression(&expr);
    RETURN_IF_GOOD(after, expr.Release(), out);
  }

  return ParsePrimary(out);
}

Parser Parser::ParseCastExpression(Result<Expr>* out) const {
  // CastExpression:
  //   "(" Type ")" UnaryExpression
  SHORT_CIRCUIT;

  Result<Token> lparen;
  Result<Type> type;
  Result<Token> rparen;
  Result<Expr> expr;

  Parser after = (*this)
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

Parser Parser::ParsePrimary(Result<Expr>* out) const {
  // Primary:
  //   "new" SingleType NewEnd
  //   PrimaryBase [ PrimaryEnd ]
  SHORT_CIRCUIT;

  // TODO: "new" SingleType NewEnd

  Result<Expr> base;
  Parser afterBase = ParsePrimaryBase(&base);
  if (!afterBase) {
    *out = move(base);
    return afterBase;
  }

  // We retain ownership IF ParsePrimaryEnd fails. If it succeeds, the
  // expectation is that it will take ownership.
  Expr* baseExpr = base.Release();
  Result<Expr> baseWithEnds;
  Parser afterEnds = afterBase.ParsePrimaryEnd(baseExpr, &baseWithEnds);
  RETURN_IF_GOOD(afterEnds, baseWithEnds.Release(), out);


  // If we couldn't parse the PrimaryEnd, then just use base; it's optional.
  return afterBase.Success(baseExpr, out);
}

Parser Parser::ParsePrimaryBase(Result<Expr>* out) const {
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
    Result<Token> litExpr;
    Parser after = ParseTokenIf(IsLiteral(), &litExpr);
    RETURN_IF_GOOD(after, new LitExpr(*litExpr.Get()), out);
  }

  {
    Result<Token> thisExpr;
    Parser after = ParseTokenIf(ExactType(K_THIS), &thisExpr);
    RETURN_IF_GOOD(after, new ThisExpr(), out);
  }

  {
    Result<Token> lparen;
    Result<Expr> expr;
    Result<Token> rparen;

    Parser after = (*this)
      .ParseTokenIf(ExactType(LPAREN), &lparen)
      .ParseExpression(&expr)
      .ParseTokenIf(ExactType(RPAREN), &rparen);
    RETURN_IF_GOOD(after, expr.Release(), out);
  }

  {
    Result<QualifiedName> name;
    Parser after = ParseQualifiedName(&name);
    RETURN_IF_GOOD(after, new NameExpr(name.Release()), out);
  }

  return Fail(MakeUnexpectedTokenError(GetNext()), out);
}

Parser Parser::ParsePrimaryEnd(Expr* base, Result<Expr>* out) const {
  // PrimaryEnd:
  //   "[" Expression "]" [ PrimaryEndNoArrayAccess ]
  //   PrimaryEndNoArrayAccess
  SHORT_CIRCUIT;

  {
    Result<Token> lbrack;
    Result<Expr> expr;
    Result<Token> rbrack;

    Parser after = (*this)
      .ParseTokenIf(ExactType(LBRACK), &lbrack)
      .ParseExpression(&expr)
      .ParseTokenIf(ExactType(RBRACK), &rbrack);
    RETURN_IF_GOOD(after, new ArrayIndexExpr(base, expr.Release()), out);
  }

  return ParsePrimaryEndNoArrayAccess(base, out);
}

Parser Parser::ParsePrimaryEndNoArrayAccess(Expr* base, Result<Expr>* out) const {
  // PrimaryEndNoArrayAccess:
  //   "." Identifier [ PrimaryEnd ]
  //   "(" [ArgumentList] ")" [ PrimaryEnd ]
  SHORT_CIRCUIT;

  {
    Result<Token> dot;
    Result<Token> ident;
    Parser after = (*this)
      .ParseTokenIf(ExactType(DOT), &dot)
      .ParseTokenIf(ExactType(IDENTIFIER), &ident);

    if (after) {
      Expr* deref = new FieldDerefExpr(base, TokenString(GetFile(), *ident.Get()), *ident.Get());
      Result<Expr> nested;
      Parser afterEnd = after.ParsePrimaryEnd(deref, &nested);
      RETURN_IF_GOOD(afterEnd, nested.Release(), out);

      return after.Success(deref, out);
    }
  }

  {
    Result<Token> lparen;
    Result<ArgumentList> args;
    Result<Token> rparen;

    Parser after = (*this)
      .ParseTokenIf(ExactType(LPAREN), &lparen)
      .ParseArgumentList(&args)
      .ParseTokenIf(ExactType(RPAREN), &rparen);

    if (after) {
      Expr* call = new CallExpr(base, move(*args.Release()));
      Result<Expr> nested;
      Parser afterEnd = after.ParsePrimaryEnd(call, &nested);
      RETURN_IF_GOOD(afterEnd, nested.Release(), out);

      return after.Success(call, out);
    }
  }

  return Fail(nullptr, out);
}

Parser Parser::ParseArgumentList(Result<ArgumentList>* out) const {
  // ArgumentList:
  //   [Expression {"," Expression}]
  SHORT_CIRCUIT;

  UniquePtrVector<Expr> args;
  Result<Expr> first;
  Parser cur = ParseExpression(&first);
  if (!cur) {
    return Success(new ArgumentList(move(args)), out);
  }
  args.Append(first.Release());

  while (true) {
    Result<Token> comma;
    Result<Expr> expr;

    Parser next = cur.ParseTokenIf(ExactType(COMMA), &comma).ParseExpression(&expr);
    if (!next) {
      return cur.Success(new ArgumentList(move(args)), out);
    }

    args.Append(expr.Release());
    cur = next;
  }
}

void Parse(const FileSet* fs, const File* file, const vector<Token>* tokens) {
  Parser parser(fs, file, tokens, 0);
  Result<Expr> result;
  parser.ParseExpression(&result);
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
