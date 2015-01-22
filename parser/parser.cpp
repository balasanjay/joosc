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

#define FAIL_IF_END(result, return_type, error) {\
  if ((result).IsAtEnd()) { return (result).template Failure<return_type>(error); } \
}

#define RETURN_IF_ERR(check) {\
  if (!(check).IsSuccess()) { \
    return (check); \
  } \
}

#define RETURN_IF_GOOD(result) {\
  if ((result).IsSuccess()) { \
    return (result); \
  } \
}

#define RETURN_WRAPPED_IF_GOOD(result, ret) {\
  if ((result).IsSuccess()) { \
    return (result).Success(ret); \
  } \
}

namespace parser {

namespace {

template<class T>
struct Result {
public:
  static Result Init(const FileSet* fs, const File* file, const vector<Token> *tokens) {
    return Result(nullptr, fs, file, tokens, 0, true, nullptr);
  }

  Result(Result<T> &&r) {
    data_.reset(r.data_.release());
    file_ = r.file_;
    tokens_ = r.tokens_;
    index_ = r.index_;
    success_ = r.success_;
    r.errors_.Release(&errors_);
  }

  void operator=(Result<T> &r) {
    data_.reset(r.data_.release());
    file_ = r.file_;
    tokens_ = r.tokens_;
    index_ = r.index_;
    success_ = r.success_;
    r.errors_.Release(&errors_);
  }

  template<class R>
  Result<R> Success(R* r) const {
    return Result<R>(r, fs_, file_, tokens_, index_, true, nullptr);
  }

  template<class R>
  Result<R> Failure(Error* err) const {
    return Result<R>(nullptr, fs_, file_, nullptr, 0, false, err);
  }

  bool IsSuccess() const {
    return success_ && !errors_.IsFatal();
  }

  bool IsAtEnd() const {
    return tokens_ == nullptr || (uint)index_ >= tokens_->size();
  }

  const FileSet* Fs() const { return fs_; }
  const File* GetFile() const { assert(file_); return file_; }

  lexer::Token GetNext() const {
    return tokens_->at(index_);
  }

  Result<T> Advance(int i = 1) const {
    // TODO: Copy errors!?
    return Result<T>(nullptr, fs_, file_, tokens_, index_ + i, success_, nullptr);
  }

  Result<Token> ParseToken(TokenType tt) const;
  Result<Token> ParseBinOp() const;
  Result<Token> ParseUnaryOp() const;
  Result<Token> ParseLiteral() const;
  Result<PrimitiveType> ParsePrimitiveType() const;
  Result<Type> ParseType() const;
  Result<Type> ParseSingleType() const;
  Result<Expr> ParseExpression() const;
  Result<Expr> ParseUnaryExpression() const;
  Result<Expr> ParseCastExpression() const;
  Result<Expr> ParsePrimary() const;
  Result<Expr> ParsePrimaryBase() const;
  Result<Expr> ParsePrimaryEnd(Expr* base) const;
  Result<Expr> ParsePrimaryEndNoArrayAccess(Expr* base) const;
  Result<QualifiedName> ParseQualifiedName() const;

  T* Get() const {
    if (!IsSuccess()) {
      throw "Get() from non-successful result.";
    }
    return data_.get();
  }

  const ErrorList& Errors() const {
    return errors_;
  }

  T* Release() {
    if (!IsSuccess()) {
      throw "Release() from non-successful result.";
    }
    return data_.release();
  }

  Result(T* data, const FileSet* fs, const File* file, const vector<Token>* tokens, int index, bool success, Error* error) : data_(data), fs_(fs), file_(file), tokens_(tokens), index_(index), success_(success) {
    if (error != nullptr) {
      errors_.Append(error);
    }
  }
  Result() : data_(nullptr), fs_(nullptr), file_(nullptr), tokens_(nullptr), index_(0), success_(false) {}

  unique_ptr<T> data_;
  const FileSet* fs_;
  const File* file_;
  const vector<lexer::Token>* tokens_;
  int index_;
  bool success_;
  ErrorList errors_;
};

Error* MakeUnexpectedTokenError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, Pos(token.pos.fileid, token.pos.begin), "UnexpectedTokenError", "Unexpected token.");
}

} // namespace

Expr* FixPrecedence(UniquePtrVector<Expr>&& owned_exprs, vector<Token> ops) {
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

template<class T>
Result<Token> Result<T>::ParseToken(TokenType tt) const {
  FAIL_IF_END(*this, Token, nullptr);
  Token t = GetNext();
  if (t.TypeInfo().Type() == tt) {
    return Advance().Success(new Token(t));
  }
  return Failure<Token>(nullptr);
}

template<class T>
Result<Token> Result<T>::ParseUnaryOp() const {
  FAIL_IF_END(*this, Token, nullptr);
  Token t = GetNext();
  if (t.TypeInfo().IsUnaryOp()) {
    return Advance().Success(new Token(t));
  }
  return Failure<Token>(nullptr);
}
template<class T>
Result<Token> Result<T>::ParseBinOp() const {
  FAIL_IF_END(*this, Token, nullptr);
  Token t = GetNext();
  if (t.TypeInfo().IsBinOp()) {
    return Advance().Success(new Token(t));
  }
  return Failure<Token>(nullptr);
}
template<class T>
Result<Token> Result<T>::ParseLiteral() const {
  FAIL_IF_END(*this, Token, nullptr);
  Token t = GetNext();
  if (t.TypeInfo().IsLiteral()) {
    return Advance().Success(new Token(t));
  }
  return Failure<Token>(nullptr);
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
  assert(file != nullptr);

  stringstream fullname;
  vector<string> parts;

  for (uint i = 0; i < tokens.size(); ++i) {
    string part = TokenString(file, tokens.at(i));
    if (i > 0) {
      fullname << '.';
    }
    fullname << part;
    parts.push_back(part);
  }

  return new QualifiedName(tokens, parts, fullname.str());
}

template<class T>
Result<QualifiedName> Result<T>::ParseQualifiedName() const {
  Result<Token> ident = ParseToken(IDENTIFIER);
  if (!ident.IsSuccess()) {
    return ident.Failure<QualifiedName>(nullptr);
  }

  vector<Token> nameParts;
  nameParts.push_back(*ident.Get());

  while (true) {
    Result<Token> nextIdent = ident.ParseToken(DOT).ParseToken(IDENTIFIER);
    if (!nextIdent.IsSuccess()) {
      return ident.Success(MakeQualifiedName(GetFile(), nameParts));
    }

    nameParts.push_back(*nextIdent.Get());
    ident = nextIdent;
  }
}

template<class T>
Result<PrimitiveType> Result<T>::ParsePrimitiveType() const {
  FAIL_IF_END(*this, PrimitiveType, nullptr);

  Token t = GetNext();
  if (t.TypeInfo().IsPrimitive()) {
    return Advance().Success(new PrimitiveType(t));
  }
  return Failure<PrimitiveType>(nullptr);
}

template<class T>
Result<Type> Result<T>::ParseSingleType() const {
  // SingleType:
  //   PrimitiveType
  //   QualifiedName

  Result<PrimitiveType> primitive = ParsePrimitiveType();
  RETURN_WRAPPED_IF_GOOD(primitive, (Type*)primitive.Release());

  Result<QualifiedName> reference = ParseQualifiedName();
  RETURN_WRAPPED_IF_GOOD(reference, (Type*)new ReferenceType(reference.Release()));

  return Failure<Type>(nullptr);
}


template<class T>
Result<Type> Result<T>::ParseType() const {
  Result<Type> single = ParseSingleType();
  Result<Token> arrayType = single.ParseToken(LBRACK).ParseToken(RBRACK);
  RETURN_WRAPPED_IF_GOOD(arrayType, (Type*)new ArrayType(single.Release()));
  return single;
}

template<class T>
Result<Expr> Result<T>::ParseCastExpression() const {
  Result<Type> castType = ParseToken(LPAREN).ParseType();
  Result<Expr> castedExpr = castType.ParseToken(RPAREN).ParseUnaryExpression();

  RETURN_WRAPPED_IF_GOOD(
      castedExpr,
      (Expr*)new CastExpr(castType.Release(),
                          castedExpr.Release()));
  return Failure<Expr>(nullptr);
}

template <class T>
Result<Expr> Result<T>::ParsePrimaryBase() const {
  // PrimaryBase:
  //   Literal
  //   "this"
  //   "(" Expression ")"
  //   ClassInstanceCreationExpression
  //   QualifiedName

  FAIL_IF_END(*this, Expr, nullptr);

  Result<Token> litExpr = ParseLiteral();
  RETURN_WRAPPED_IF_GOOD(litExpr, (Expr*)new LitExpr(*litExpr.Get()));

  Result<Token> thisExpr = ParseToken(K_THIS);
  RETURN_WRAPPED_IF_GOOD(thisExpr, (Expr*)new ThisExpr());

  Result<Expr> nested = ParseToken(LPAREN).ParseExpression();
  Result<Token> afterRParen = nested.ParseToken(RPAREN);
  RETURN_WRAPPED_IF_GOOD(afterRParen, nested.Release());

  // TODO: ClassInstanceCreationExpression.

  Result<QualifiedName> name = ParseQualifiedName();
  RETURN_WRAPPED_IF_GOOD(name, (Expr*)new NameExpr(name.Release()));

  return Failure<Expr>(MakeUnexpectedTokenError(Fs(), GetNext()));
}

template <class T>
Result<Expr> Result<T>::ParsePrimary() const {
  // Primary:
  //   PrimaryBase [ PrimaryEnd ]
  //   ArrayCreationExpression [ PrimaryEndNoArrayAccess ]

  Result<Expr> base = ParsePrimaryBase();
  RETURN_IF_ERR(base);

  Expr* baseExpr = base.Release();
  Result<Expr> baseWithEnds = base.ParsePrimaryEnd(baseExpr);
  RETURN_IF_GOOD(baseWithEnds);

  // TODO: ArrayCreationExpression [ PrimaryEndNoArrayAccess ]

  return base.Success(baseExpr);
}

template <class T>
Result<Expr> Result<T>::ParsePrimaryEnd(Expr* base) const {
  // PrimaryEnd:
  //   "[" Expression "]" [ PrimaryEndNoArrayAccess ]
  //   PrimaryEndNoArrayAccess

  if (!IsSuccess()) {
     return Failure<Expr>(nullptr);
  }
  Result<Expr> arrayIndex = ParseToken(LBRACK).ParseExpression();
  Result<Token> afterRBrack = arrayIndex.ParseToken(RBRACK);

  RETURN_WRAPPED_IF_GOOD(afterRBrack, (Expr*)new ArrayIndexExpr(base, arrayIndex.Release()));

  return ParsePrimaryEndNoArrayAccess(base);
}

template <class T>
Result<Expr> Result<T>::ParsePrimaryEndNoArrayAccess(Expr* base) const {
  // PrimaryEndNoArrayAccess:
  //   "." Identifier [ PrimaryEnd ]
  //   "(" [ArgumentList] ")" [ PrimaryEnd ]

  if (!IsSuccess()) {
     return Failure<Expr>(nullptr);
  }
  Result<Token> field = ParseToken(DOT).ParseToken(IDENTIFIER);
  Result<Expr> fieldAccess = field.ParsePrimaryEnd(base);
  RETURN_WRAPPED_IF_GOOD(
      fieldAccess,
      (Expr*)new FieldDerefExpr(
        base,
        TokenString(GetFile(), *field.Get()),
        *field.Get()));

  // TODO: "(" [ArgumentList] ")" [ PrimaryEnd ]

  return Failure<Expr>(nullptr);
}

template <class T>
Result<Expr> Result<T>::ParseUnaryExpression() const {
  // UnaryExpression:
  //   "-" UnaryExpression
  //   "!" UnaryExpression
  //   CastExpression
  //   Primary

  if (!IsSuccess()) {
    return Failure<Expr>(nullptr);
  }

  Result<Token> unaryOp = ParseUnaryOp();
  Result<Expr> nested = unaryOp.ParseUnaryExpression();
  RETURN_WRAPPED_IF_GOOD(nested, (Expr*)new UnaryExpr(*unaryOp.Get(), nested.Release()));

  Result<Expr> castExpr = ParseCastExpression();
  RETURN_IF_GOOD(castExpr);

  Result<Expr> p = ParsePrimary();
  return p;
}

template <class T>
Result<Expr> Result<T>::ParseExpression() const {
  UniquePtrVector<Expr> exprs;
  vector<Token> operators;

  Result<Expr> expr = ParseUnaryExpression();
  RETURN_IF_ERR(expr);
  exprs.Append(expr.Release());

  while (true) {
    Result<Token> binOp = expr.ParseBinOp();
    Result<Expr> nextExpr = binOp.ParseUnaryExpression();

    if (!nextExpr.IsSuccess()) {
      return expr.Success(FixPrecedence(std::move(exprs), operators));
    }

    operators.push_back(*binOp.Get());
    exprs.Append(nextExpr.Release());
    expr = nextExpr;
  }
}

void Parse(const FileSet* fs, const File* file, const vector<Token>* tokens) {
  Result<Expr> result = Result<Expr>::Init(fs, file, tokens).ParseExpression();
  if (result.IsSuccess()) {
    if (result.IsAtEnd()) {
      result.Get()->PrintTo(&std::cout);
      std::cout << '\n';
    } else {
      std::cout << "Failed to parse all of the input.\n";
    }
  } else {
    std::cout << "Failed.\n";
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
