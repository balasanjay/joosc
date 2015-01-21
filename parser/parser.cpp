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

#define RETURN_IF_ERR(check) {\
  if (!(check).IsSuccess()) { \
    return (check); \
  } \
}

#define RETURN_IF_GOOD(check, expr) {\
  if ((check).IsSuccess()) { \
    return (expr); \
  } \
}

namespace parser {

namespace {
struct State final {
public:
  State(const File* file, const vector<lexer::Token>* tokens, int index) : file_(file), tokens_(tokens), index_(index) {}
  State(State old, int advance) : file_(old.file_), tokens_(old.tokens_), index_(old.index_ + advance) {}

  bool IsAtEnd() const {
    return tokens_ == nullptr || (uint)index_ >= tokens_->size();
  }

  lexer::Token GetNext() const {
    return tokens_->at(index_);
  }

  State Advance(int i = 1) const {
    return State(*this, i);
  }

  const File* GetFile() const { return file_; }

private:
  const File* file_;
  const vector<lexer::Token>* tokens_;
  int index_;
};

template<class T>
struct Result final {
public:
  static Result<T> Success(T* t, State state) {
    return Result<T>(t, state);
  }

  static Result<T> Failure() {
    return Result<T>();
  }

  Result(Result&& other) = default;

  bool IsSuccess() const {
    return success_;
  }

  T* Get() const {
    if (!IsSuccess()) {
      throw "Get() from non-successful result.";
    }
    return data_.get();
  }

  State NewState() const {
    return state_;
  }

  T* Release() {
    if (!IsSuccess()) {
      throw "Release() from non-successful result.";
    }
    return data_.release();
  }

private:
  Result(T* data, State state) : data_(data), state_(state), success_(true) {}
  Result() : state_(nullptr, nullptr, 0), success_(false) {}

  unique_ptr<T> data_;
  State state_;
  bool success_;
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

Result<Expr> ParseExpression(State state);
Result<Expr> ParseUnaryExpression(State state);
Result<Expr> ParsePrimaryEnd(State state, Expr* base);
Result<Expr> ParsePrimaryEndNoArrayAccess(State state, Expr* base);

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

Result<QualifiedName> ParseQualifiedName(State state) {
  if (state.IsAtEnd() || state.GetNext().type != IDENTIFIER) {
    return Result<QualifiedName>::Failure();
  }

  vector<Token> name;
  name.push_back(state.GetNext());

  State cur = state.Advance();
  while (true) {
    if (cur.IsAtEnd() || cur.GetNext().type != DOT) {
      return Result<QualifiedName>::Success(MakeQualifiedName(state.GetFile(), name), cur);
    }

    State afterdot = cur.Advance();
    if (afterdot.IsAtEnd() || afterdot.GetNext().type != IDENTIFIER) {
      return Result<QualifiedName>::Success(MakeQualifiedName(state.GetFile(), name), cur);
    }

    name.push_back(cur.GetNext());
    name.push_back(afterdot.GetNext());
    cur = afterdot.Advance();
  }
}

Result<PrimitiveType> ParsePrimitiveType(State state) {
  if (state.IsAtEnd()) {
    return Result<PrimitiveType>::Failure();
  }

  if (state.GetNext().TypeInfo().IsPrimitive()) {
    return Result<PrimitiveType>::Success(new PrimitiveType(state.GetNext()), state.Advance());
  }

  return Result<PrimitiveType>::Failure();
}

Result<Type> ParseSingleType(State state) {
  // SingleType:
  //   PrimitiveType
  //   QualifiedName

  {
    Result<PrimitiveType> primitive = ParsePrimitiveType(state);
    if (primitive.IsSuccess()) {
      return Result<Type>::Success(primitive.Release(), primitive.NewState());
    }
  }

  {
    Result<QualifiedName> reference = ParseQualifiedName(state);
    if (reference.IsSuccess()) {
      return Result<Type>::Success(new ReferenceType(reference.Release()), reference.NewState());
    }
  }

  return Result<Type>::Failure();
}

Result<Type> ParseType(State state) {
  Result<Type> single = ParseSingleType(state);
  RETURN_IF_ERR(single);

  State aftertype = single.NewState();
  if (aftertype.IsAtEnd() || aftertype.GetNext().type != LBRACK) {
    return single;
  }

  State afterlbrack = aftertype.Advance();
  if (afterlbrack.IsAtEnd() || afterlbrack.GetNext().type != RBRACK) {
    return single;
  }

  return Result<Type>::Success(new ArrayType(single.Release()), afterlbrack.Advance());
}

Result<Expr> ParseCastExpression(State state) {
  if (state.IsAtEnd() || state.GetNext().type != LPAREN) {
    return Result<Expr>::Failure();
  }

  Result<Type> casttype = ParseType(state.Advance());
  if (!casttype.IsSuccess()) {
    return Result<Expr>::Failure();
  }

  State aftertype = casttype.NewState();
  if (aftertype.IsAtEnd() || aftertype.GetNext().type != RPAREN) {
    return Result<Expr>::Failure();
  }

  Result<Expr> castedExpr = ParseUnaryExpression(aftertype.Advance());
  RETURN_IF_ERR(castedExpr);

  return Result<Expr>::Success(
      new CastExpr(casttype.Release(), castedExpr.Release()),
      castedExpr.NewState()
  );
}

Result<Expr> ParsePrimaryBase(State state) {
  // PrimaryBase:
  //   Literal
  //   "this"
  //   "(" Expression ")"
  //   ClassInstanceCreationExpression
  //   QualifiedName

  if (state.IsAtEnd()) {
    return Result<Expr>::Failure();
  }

  if (state.GetNext().TypeInfo().IsLiteral()) {
    return Result<Expr>::Success(new LitExpr(state.GetNext()), state.Advance());
  }

  if (state.GetNext().type == K_THIS) {
    return Result<Expr>::Success(new ThisExpr(), state.Advance());
  }

  if (state.GetNext().type == LPAREN) {
    Result<Expr> nested = ParseExpression(state.Advance());
    RETURN_IF_ERR(nested);

    State next = nested.NewState();
    if (next.IsAtEnd() || next.GetNext().type != RPAREN) {
      return Result<Expr>::Failure();
    }

    return Result<Expr>::Success(nested.Release(), next.Advance());
  }

  // TODO: ClassInstanceCreationExpression.

  {
    Result<QualifiedName> name = ParseQualifiedName(state);
    if (name.IsSuccess()) {
      return Result<Expr>::Success(new NameExpr(name.Release()), name.NewState());
    }
  }

  throw;
}

Result<Expr> ParsePrimary(State state) {
  // Primary:
  //   PrimaryBase [ PrimaryEnd ]
  //   ArrayCreationExpression [ PrimaryEndNoArrayAccess ]

  Result<Expr> base = ParsePrimaryBase(state);
  RETURN_IF_GOOD(base, ParsePrimaryEnd(base.NewState(), base.Release()));

  // TODO: ArrayCreationExpression [ PrimaryEndNoArrayAccess ]
  throw;
}

Result<Expr> ParsePrimaryEnd(State state, Expr* base) {
  // PrimaryEnd:
  //   "[" Expression "]" [ PrimaryEndNoArrayAccess ]
  //   PrimaryEndNoArrayAccess

  if (state.IsAtEnd()) {
    return Result<Expr>::Success(base, state);
  }

  if (state.GetNext().type == LBRACK) {
    Result<Expr> index = ParseExpression(state.Advance());
    if (!index.IsSuccess()) {
      return Result<Expr>::Success(base, state);
    }

    State afterIndex = index.NewState();
    if (afterIndex.IsAtEnd() || afterIndex.GetNext().type != RBRACK) {
      return Result<Expr>::Success(base, state);
    }

    return ParsePrimaryEndNoArrayAccess(
        afterIndex.Advance(),
        new ArrayIndexExpr(base, index.Release())
    );
  }

  return ParsePrimaryEndNoArrayAccess(state, base);
}

FieldDerefExpr* MakeFieldDeref(State state, Expr* base) {
  assert(!state.IsAtEnd() && state.GetNext().type == IDENTIFIER);
  string fieldname = TokenString(state.GetFile(), state.GetNext());
  return new FieldDerefExpr(base, fieldname, state.GetNext());
}

Result<Expr> ParsePrimaryEndNoArrayAccess(State state, Expr* base) {
  // PrimaryEndNoArrayAccess:
  //   "." Identifier [ PrimaryEnd ]
  //   "(" [ArgumentList] ")" [ PrimaryEnd ]

  // NOTE: both productions have at least two tokens, so we pre-advance and
  // check IsAtEnd().
  State afterFirst = state.Advance();

  if (state.IsAtEnd() || afterFirst.IsAtEnd()) {
    return Result<Expr>::Success(base, state);
  }

  if (state.GetNext().type == DOT && afterFirst.GetNext().type == IDENTIFIER) {
    return ParsePrimaryEnd(
        afterFirst.Advance(),
        MakeFieldDeref(afterFirst, base)
    );
  }

  // TODO: "(" [ArgumentList] ")" [ PrimaryEnd ]

  return Result<Expr>::Success(base, state);
}


Result<Expr> ParseUnaryExpression(State state) {
  // UnaryExpression:
  //   "-" UnaryExpression
  //   "!" UnaryExpression
  //   CastExpression
  //   Primary

  if (state.IsAtEnd()) {
    return Result<Expr>::Failure();
  }

  if (state.GetNext().TypeInfo().IsUnaryOp()) {
    Result<Expr> nested = ParseUnaryExpression(state.Advance());
    RETURN_IF_ERR(nested);

    return Result<Expr>::Success(
        new UnaryExpr(state.GetNext(), nested.Release()),
        nested.NewState()
    );
  }

  Result<Expr> castExpr = ParseCastExpression(state);
  if (castExpr.IsSuccess()) {
    return castExpr;
  }

  return ParsePrimary(state);
}

Result<Expr> ParseExpression(State state) {
  vector<Expr*> exprs;
  vector<Token> operators;

  State cur = state;

  while (true) {
    Result<Expr> nextExpr = ParseUnaryExpression(cur);
    if (!nextExpr.IsSuccess()) {
      // TODO: we leak exprs.
      return nextExpr;
    }

    exprs.push_back(nextExpr.Release());
    cur = nextExpr.NewState();

    if (cur.IsAtEnd() || !cur.GetNext().TypeInfo().IsBinOp()) {
      return Result<Expr>::Success(
          FixPrecedence(exprs, operators),
          cur);
    }

    operators.push_back(cur.GetNext());
    cur = cur.Advance();
  }
}

void Parse(const File* file, const vector<Token>* tokens) {
  State state(file, tokens, 0);
  Result<Expr> result = ParseExpression(state);
  assert(result.IsSuccess());
  result.Get()->PrintTo(&std::cout);
  std::cout << '\n';
}

// TODO: in for-loop initializers, for-loop incrementors, and top-level
// statements, we must ensure that they are either assignment, method
// invocation, or class creation, not other types of expressions (like boolean
// ops).

} // namespace parser
