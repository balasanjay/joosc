#include "parser/ast.h"
#include "lexer/lexer.h"
#include <iostream>

using lexer::ADD;
using lexer::K_THIS;
using lexer::ASSG;
using lexer::IDENTIFIER;
using lexer::INTEGER;
using lexer::LPAREN;
using lexer::MUL;
using lexer::RPAREN;
using lexer::Token;
using lexer::TokenType;
using std::cerr;

namespace parser {

namespace {
struct State {
public:
  State(const vector<lexer::Token>* tokens, int index) : tokens_(tokens), index_(index) {}
  State(State old, int advance) : tokens_(old.tokens_), index_(old.index_ + advance) {}

  bool IsAtEnd() const {
    return tokens_ == nullptr || (uint)index_ >= tokens_->size();
  }

  lexer::Token GetNext() const {
    return tokens_->at(index_);
  }

  State Advance(int i = 1) const {
    return State(*this, i);
  }

private:
  const vector<lexer::Token>* tokens_;
  int index_;
};

template<class T>
struct Result {
public:
  static Result<T> Success(T* t, State state) {
    return Result<T>(t, state);
  }

  static Result<T> Failure() {
    return Result<T>();
  }

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
  Result() : state_(nullptr, 0), success_(false) {}

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
          (op.type == ASSG && TokenTypeBinOpPrec(op.type) >= TokenTypeBinOpPrec(opstack.rbegin()->type)) ||
          (op.type != ASSG && TokenTypeBinOpPrec(op.type) > TokenTypeBinOpPrec(opstack.rbegin()->type))) {
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

Result<Expr> ParseCastExpression(State state) {
  if (state.IsAtEnd() || state.GetNext().type != LPAREN) {
    return Result<Expr>::Failure();
  }

  // HACK: only casting to identifiers.
  State afterLParen = state.Advance();
  if (afterLParen.IsAtEnd() || afterLParen.GetNext().type != IDENTIFIER) {
    return Result<Expr>::Failure();
  }

  State afterIdent = afterLParen.Advance();
  if (afterIdent.IsAtEnd() || afterIdent.GetNext().type != RPAREN) {
    return Result<Expr>::Failure();
  }

  State afterRParen = afterIdent.Advance();
  return ParseUnaryExpression(afterRParen);
}

Result<Expr> ParsePrimaryBase(State state) {
  // PrimaryBase:
  //   QualifiedName
  //   Literal
  //   "this"
  //   "(" Expression ")"
  //   ClassInstanceCreationExpression

  if (state.IsAtEnd()) {
    return Result<Expr>::Failure();
  }

  // TODO: QualifiedName

  // TODO: literals other than integer.
  if (state.GetNext().type == INTEGER) {
    return Result<Expr>::Success(new ConstExpr(), state.Advance());
  }

  if (state.GetNext().type == K_THIS) {
    return Result<Expr>::Success(new ThisExpr(), state.Advance());
  }

  if (state.GetNext().type == LPAREN) {
    Result<Expr> nested = ParseExpression(state.Advance(1));
    if (!nested.IsSuccess()) {
      return Result<Expr>::Failure();
    }

    State next = nested.NewState();
    if (!next.IsAtEnd() && next.GetNext().type == RPAREN) {
      return Result<Expr>::Success(nested.Release(), next.Advance(1));
    } else {
      return Result<Expr>::Failure();
    }
  }

  // TODO: ClassInstanceCreationExpression.
  throw;
}

Result<Expr> ParsePrimary(State state) {
  // Primary:
  //   PrimaryBase [ PrimaryEnd ]
  //   ArrayCreationExpression [ PrimaryEndNoArrayAccess ]

  Result<Expr> base = ParsePrimaryBase(state);
  if (base.IsSuccess()) {
    // TODO: optional PrimaryEnd.
    return base;
  }

  // TODO: ArrayCreationExpression [ PrimaryEndNoArrayAccess ]
  throw;
}

Result<Expr> ParseUnaryExpression(State state) {
  // UnaryExpressionession:
  //   "-" UnaryExpressionession
  //   "!" UnaryExpressionession
  //   CastExpression
  //   Primary

  if (state.IsAtEnd()) {
    return Result<Expr>::Failure();
  }

  if (TokenTypeIsUnaryOp(state.GetNext().type)) {
    Result<Expr> nested = ParseUnaryExpression(state.Advance());
    if (!nested.IsSuccess()) {
      return nested;
    }
    return Result<Expr>::Success(new UnaryExpr(state.GetNext(), nested.Release()), nested.NewState());
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

    if (cur.IsAtEnd()) {
      return Result<Expr>::Success(
          FixPrecedence(exprs, operators),
          cur);
    }

    if (TokenTypeIsBinOp(cur.GetNext().type)) {
      operators.push_back(cur.GetNext());
      cur = cur.Advance();
      continue;
    }

    return Result<Expr>::Success(
        FixPrecedence(exprs, operators),
        cur);
  }
}

void Parse(const vector<Token>* tokens) {
  State state(tokens, 0);
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
