#include "parser/ast.h"
#include "lexer/lexer.h"

using base::File;
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

#define FAIL_IF_END(result, return_type) {\
  if ((result).IsAtEnd()) { return (result).template Failure<return_type>(); } \
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

template <class T>
T deleteReturnVal(T* ptr) {
  T t = *ptr;
  delete ptr;
  return t;
}

template<class T>
struct Result {
public:
  static Result Init(const File* file, const vector<Token> *tokens) {
    return Result(nullptr, file, tokens, 0, true);
  }

  Result(Result<T> &&r) {
    data_.reset(r.data_.release());
    file_ = r.file_;
    tokens_ = r.tokens_;
    index_ = r.index_;
    success_ = r.success_;
  }

  void operator=(Result<T> &r) {
    data_.reset(r.data_.release());
    file_ = r.file_;
    tokens_ = r.tokens_;
    index_ = r.index_;
    success_ = r.success_;
  }

  template<class R>
  Result<R> Success(R* r) const {
    return Result<R>(r, file_, tokens_, index_, true);
  }

  // TODO: Error.
  template<class R>
  Result<R> Failure() const {
    return Result<R>(nullptr, file_, nullptr, 0, false);
  }

  bool IsSuccess() const {
    return success_;
  }

  bool IsAtEnd() const {
    return tokens_ == nullptr || (uint)index_ >= tokens_->size();
  }

  const File* GetFile() const { assert(file_); return file_; }

  lexer::Token GetNext() const {
    return tokens_->at(index_);
  }

  Result<T> Advance(int i = 1) const {
    return Result<T>(nullptr, file_, tokens_, index_ + i, success_);
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


  T* Release() {
    if (!IsSuccess()) {
      throw "Release() from non-successful result.";
    }
    return data_.release();
  }

  Result(T* data, const File* file, const vector<Token>* tokens, int index, bool success) : data_(data), file_(file), tokens_(tokens), index_(index), success_(success) {}
  Result() : data_(nullptr), file_(nullptr), tokens_(nullptr), index_(0), success_(false) {}

//private:
  unique_ptr<T> data_;
  const File* file_;
  const vector<lexer::Token>* tokens_;
  int index_;
  bool success_;
  // TODO: Error.
};

} // namespace

Expr* FixPrecedence(vector<Expr*> exprs, vector<Token> ops) {
  assert(exprs.size() == ops.size() + 1);
  vector<Expr*> outstack;
  vector<Token> opstack;

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
  FAIL_IF_END(*this, Token);
  Token t = GetNext();
  if (t.TypeInfo().Type() == tt) {
    return Advance().Success(new Token(t));
  }
  return Failure<Token>();
}

template<class T>
Result<Token> Result<T>::ParseUnaryOp() const {
  FAIL_IF_END(*this, Token);
  Token t = GetNext();
  if (t.TypeInfo().IsUnaryOp()) {
    return Advance().Success(new Token(t));
  }
  return Failure<Token>();
}
template<class T>
Result<Token> Result<T>::ParseBinOp() const {
  FAIL_IF_END(*this, Token);
  Token t = GetNext();
  if (t.TypeInfo().IsBinOp()) {
    return Advance().Success(new Token(t));
  }
  return Failure<Token>();
}
template<class T>
Result<Token> Result<T>::ParseLiteral() const {
  FAIL_IF_END(*this, Token);
  Token t = GetNext();
  if (t.TypeInfo().IsLiteral()) {
    return Advance().Success(new Token(t));
  }
  return Failure<Token>();
}

QualifiedName* MakeQualifiedName(const File *file, const vector<Token>& tokens) {
  assert(tokens.size() > 0);
  assert(file != nullptr);

  stringstream fullname;
  vector<string> parts;

  for (uint i = 0; i < tokens.size(); ++i) {
    stringstream partname;
    Token token = tokens.at(i);
    //throw;
    std::cout << "MakeQualifiedName [" << token.pos.begin << ", " << token.pos.end << "). File " << file->Basename() << " has " << file->Size() << std::endl;
    for (int j = token.pos.begin; j < token.pos.end; ++j) {
      partname << file->At(j);
    }

    string part = partname.str();
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
  //std::cout << "ParseQualifiedName " << GetFile()->Basename() << std::endl;
  Result<Token> ident = ParseToken(IDENTIFIER);
  if (!ident.IsSuccess()) {
    return ident.Failure<QualifiedName>();
  }

  vector<Token> nameParts;
  nameParts.push_back(deleteReturnVal(ident.Release()));

  while (true) {
    Result<Token> nextIdent = ident.ParseToken(DOT).ParseToken(IDENTIFIER);
    if (!nextIdent.IsSuccess()) {
      return ident.Success(MakeQualifiedName(GetFile(), nameParts));
    }

    nameParts.push_back(deleteReturnVal(nextIdent.Release()));
    ident = nextIdent;
  }
}

template<class T>
Result<PrimitiveType> Result<T>::ParsePrimitiveType() const {
  FAIL_IF_END(*this, PrimitiveType);

  Token t = GetNext();
  if (t.TypeInfo().IsPrimitive()) {
    return Advance().Success(new PrimitiveType(t));
  }
  return Failure<PrimitiveType>();
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

  return Failure<Type>();
}


template<class T>
Result<Type> Result<T>::ParseType() const {
  //std::cout << "ParseType " << GetFile()->Basename() << std::endl;
  Result<Type> single = ParseSingleType();
  Result<Token> arrayType = single.ParseToken(LBRACK).ParseToken(RBRACK);
  RETURN_WRAPPED_IF_GOOD(arrayType, (Type*)new ArrayType(single.Release()));
  return single;
}

template<class T>
Result<Expr> Result<T>::ParseCastExpression() const {
  //std::cout << "ParseCastExpression " << GetFile()->Basename() << std::endl;
  Result<Type> castType = ParseToken(LPAREN).ParseType();
  Result<Expr> castedExpr = castType.ParseToken(RPAREN).ParseUnaryExpression();

  RETURN_WRAPPED_IF_GOOD(
      castedExpr,
      (Expr*)new CastExpr(castType.Release(),
                          castedExpr.Release()));
  return Failure<Expr>();
}

template <class T>
Result<Expr> Result<T>::ParsePrimaryBase() const {
  // PrimaryBase:
  //   Literal
  //   "this"
  //   "(" Expression ")"
  //   ClassInstanceCreationExpression
  //   QualifiedName

  //FAIL_IF_END();

  Result<Token> litExpr = ParseLiteral();
  RETURN_WRAPPED_IF_GOOD(litExpr, (Expr*)new LitExpr(*litExpr.Get()));

  Result<Token> thisExpr = ParseToken(K_THIS);
  RETURN_WRAPPED_IF_GOOD(thisExpr, (Expr*)new ThisExpr());

  Result<Expr> nested = ParseToken(LPAREN).ParseExpression();
  Result<Token> afterRParen = nested.ParseToken(RPAREN);
  RETURN_WRAPPED_IF_GOOD(afterRParen, nested.Release());

  // TODO: ClassInstanceCreationExpression.

  if (IsSuccess()) {
    std::cout << "IsSuccess\n";
  } else {
    std::cout << "IsFail\n";
  }
  Result<QualifiedName> name = ParseQualifiedName();
  RETURN_WRAPPED_IF_GOOD(name, (Expr*)new NameExpr(name.Release()));

  return Failure<Expr>();
}

template <class T>
Result<Expr> Result<T>::ParsePrimary() const {
  // Primary:
  //   PrimaryBase [ PrimaryEnd ]
  //   ArrayCreationExpression [ PrimaryEndNoArrayAccess ]

  Result<Expr> base = ParsePrimaryBase();
  RETURN_IF_ERR(base);

  Result<Expr> baseWithEnds = base.ParsePrimaryEnd(base.Release());
  RETURN_IF_GOOD(baseWithEnds);

  // TODO: ArrayCreationExpression [ PrimaryEndNoArrayAccess ]

  return base;
}

template <class T>
Result<Expr> Result<T>::ParsePrimaryEnd(Expr* base) const {
  // PrimaryEnd:
  //   "[" Expression "]" [ PrimaryEndNoArrayAccess ]
  //   PrimaryEndNoArrayAccess

  Result<Expr> arrayIndex = ParseToken(LBRACK).ParseExpression();
  Result<Token> afterRBrack = arrayIndex.ParseToken(RBRACK);

  RETURN_WRAPPED_IF_GOOD(afterRBrack, (Expr*)new ArrayIndexExpr(base, arrayIndex.Release()));

  return ParsePrimaryEndNoArrayAccess(base);
}

template <class T>
Result<Expr> Result<T>::ParsePrimaryEndNoArrayAccess(Expr* base) const {
  // PrimaryEndNoArrayAccess:
  //   "(" [ArgumentList] ")" [ PrimaryEnd ]
  //   "." Identifier [ PrimaryEnd ]

  // TODO: implement this.
  return Success(base);

  // TODO: Free base if we fail.
}

template <class T>
Result<Expr> Result<T>::ParseUnaryExpression() const {
  // UnaryExpression:
  //   "-" UnaryExpression
  //   "!" UnaryExpression
  //   CastExpression
  //   Primary

  if (!IsSuccess()) {
    return Failure<Expr>();
  }
  //std::cout << "ParseUnaryExpression " << GetFile()->Basename() << std::endl;
  Result<Token> unaryOp = ParseUnaryOp();
  Result<Expr> nested = unaryOp.ParseUnaryExpression();
  RETURN_WRAPPED_IF_GOOD(nested, (Expr*)new UnaryExpr(deleteReturnVal(unaryOp.Release()), nested.Release()));

  Result<Expr> castExpr = ParseCastExpression();
  RETURN_IF_GOOD(castExpr);

  return ParsePrimary();
}

template <class T>
Result<Expr> Result<T>::ParseExpression() const {
  vector<Expr*> exprs;
  vector<Token> operators;

  Result<Expr> expr = ParseUnaryExpression();
  RETURN_IF_ERR(expr);
  exprs.push_back(expr.Release());

  while (true) {
    Result<Token> binOp = expr.ParseBinOp();
    Result<Expr> nextExpr = binOp.ParseUnaryExpression();

    if (!nextExpr.IsSuccess()) {
      return expr.Success(FixPrecedence(exprs, operators));
    }

    operators.push_back(deleteReturnVal(binOp.Release()));
    exprs.push_back(nextExpr.Release());
    expr = nextExpr;
  }
}

void Parse(const File* file, const vector<Token>* tokens) {
  std::cout << "Parse " << file->Basename() << std::endl;
  Result<Expr> result = Result<Expr>::Init(file, tokens).ParseExpression();
  assert(result.IsSuccess());
  assert(result.IsAtEnd());
  result.Get()->PrintTo(&std::cout);
  std::cout << '\n';
}


// TODO: in for-loop initializers, for-loop incrementors, and top-level
// statements, we must ensure that they are either assignment, method
// invocation, or class creation, not other types of expressions (like boolean
// ops).

} // namespace parser
