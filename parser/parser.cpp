#include "base/unique_ptr_vector.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "parser/parser_internal.h"
#include "parser/print_visitor.h"
#include "weeder/assignment_visitor.h"
#include "weeder/call_visitor.h"
#include "weeder/type_visitor.h"
#include "weeder/modifier_visitor.h"

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
using lexer::K_CLASS;
using lexer::K_ELSE;
using lexer::K_EXTENDS;
using lexer::K_FOR;
using lexer::K_IF;
using lexer::K_IMPLEMENTS;
using lexer::K_INTERFACE;
using lexer::K_NEW;
using lexer::K_RETURN;
using lexer::K_THIS;
using lexer::LBRACE;
using lexer::LBRACK;
using lexer::LPAREN;
using lexer::MUL;
using lexer::RBRACE;
using lexer::RBRACK;
using lexer::RPAREN;
using lexer::SEMI;
using lexer::Token;
using lexer::TokenType;
using parser::internal::ConvertError;
using parser::internal::Result;
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
// ScopedPrint _scoped_print(string("Entering ") + __FUNCTION__, string("Leaving ") + __FUNCTION__)

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
struct IsModifier {
  bool operator()(const Token& t) { return t.TypeInfo().IsModifier(); }
};


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

} // namespace

Error* Parser::MakeUnexpectedTokenError(Token token) const {
  // TODO: say what you expected instead.
  return MakeSimplePosRangeError(fs_, Pos(token.pos.fileid, token.pos.begin), "UnexpectedTokenError", "Unexpected token.");
}

Error* Parser::MakeDuplicateModifierError(Token token) const {
  return MakeSimplePosRangeError(fs_, token.pos, "DuplicateModifierError", "Duplicate modifier.");
}

Error* Parser::MakeParamRequiresNameError(Token token) const {
  return MakeSimplePosRangeError(fs_, token.pos, "ParamRequiresNameError", "A parameter requires a type and a name.");
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

  while (cur.IsNext(DOT)) {
    Result<Token> dot;
    Result<Token> nextIdent;
    Parser next = cur
      .ParseTokenIf(ExactType(DOT), &dot)
      .ParseTokenIf(ExactType(IDENTIFIER), &nextIdent);
    if (!next) {
      ErrorList errors;
      FirstOf(&errors, &dot, &nextIdent);
      return Fail(move(errors), out);
    }

    tokens.push_back(*dot.Get());
    tokens.push_back(*nextIdent.Get());
    cur = next;
  }
  return cur.Success(MakeQualifiedName(GetFile(), tokens), out);
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

  *out = ConvertError<Token, Type>(move(primitive));
  return Fail();
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

  if (IsNext(IDENTIFIER)) {
    Result<QualifiedName> reference;
    Parser after = ParseQualifiedName(&reference);
    RETURN_IF_GOOD(after, new ReferenceType(reference.Release()), out);

    *out = ConvertError<QualifiedName, Type>(move(reference));
    return Fail();
  }

  if (IsAtEnd()) {
    return Fail(MakeUnexpectedEOFError(), out);
  }

  return Fail(MakeUnexpectedTokenError(GetNext()), out);
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

  if (afterSingle.IsNext(LBRACK)) {
    Result<Token> lbrack;
    Result<Token> rbrack;

    Parser afterArray = afterSingle
      .ParseTokenIf(ExactType(LBRACK), &lbrack)
      .ParseTokenIf(ExactType(RBRACK), &rbrack);
    RETURN_IF_GOOD(afterArray, new ArrayType(single.Release()), out);

    *out = ConvertError<Token, Type>(move(rbrack));
    return Fail();
  }

  // If we didn't find brackets, then just use the initial expression on its
  // own.
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

  while (cur.IsNext(IsBinOp())) {
    Result<Token> binOp;
    Result<Expr> nextExpr;

    Parser next = cur.ParseTokenIf(IsBinOp(), &binOp).ParseUnaryExpression(&nextExpr);

    if (!next) {
      ErrorList errors;
      FirstOf(&errors, &binOp, &nextExpr);
      return Fail(move(errors), out);
    }

    operators.push_back(*binOp.Get());
    exprs.Append(nextExpr.Release());
    cur = next;
  }

  return cur.Success(FixPrecedence(move(exprs), operators), out);
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

  if (IsNext(IsUnaryOp())) {
    Result<Token> unaryOp;
    Result<Expr> expr;
    Parser after = ParseTokenIf(IsUnaryOp(), &unaryOp).ParseUnaryExpression(&expr);
    RETURN_IF_GOOD(after, new UnaryExpr(*unaryOp.Get(), expr.Release()), out);

    ErrorList errors;
    FirstOf(&errors, &unaryOp, &expr);
    return Fail(move(errors), out);
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

  if (IsNext(K_NEW)) {
    return ParseNewExpression(out);
  }

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

Parser Parser::ParseNewExpression(Result<Expr>* out) const {
  // "new" SingleType NewEnd
  //
  // NewEnd:
  //   "(" ArgumentList ")" [ PrimaryEnd ]
  //   "[" [Expression] "] [ PrimaryEndNoArrayAccess ]
  SHORT_CIRCUIT;

  Result<Token> newTok;
  Result<Type> type;
  Parser afterType = ParseTokenIf(ExactType(K_NEW), &newTok).ParseSingleType(&type);
  if (!afterType) {
    // Collect the first error, and use that.
    ErrorList errors;
    FirstOf(&errors, &newTok, &type);
    return Fail(move(errors), out);
  }

  if (afterType.IsAtEnd()) {
    return Fail(MakeUnexpectedEOFError(), out);
  }

  if (!afterType.IsNext(LPAREN) && !afterType.IsNext(LBRACK)) {
    return Fail(MakeUnexpectedTokenError(afterType.GetNext()), out);
  }

  if (afterType.IsNext(LPAREN)) {
    Result<Token> lparen;
    Result<ArgumentList> args;
    Result<Token> rparen;
    Parser afterCall = afterType
      .ParseTokenIf(ExactType(LPAREN), &lparen)
      .ParseArgumentList(&args)
      .ParseTokenIf(ExactType(RPAREN), &rparen);

    if (!afterCall) {
      // Collect the first error, and use that.
      ErrorList errors;
      FirstOf(&errors, &lparen, &args, &rparen);
      return Fail(move(errors), out);
    }

    Expr* newExpr = new NewClassExpr(*newTok.Get(), type.Release(), args.Release());
    Result<Expr> nested;
    Parser afterEnd = afterCall.ParsePrimaryEnd(newExpr, &nested);
    RETURN_IF_GOOD(afterEnd, nested.Release(), out);

    return afterCall.Success(newExpr, out);
  }

  assert(afterType.IsNext(LBRACK));
  Expr* sizeExpr = nullptr;
  Parser after = *this;

  if (afterType.Advance().IsNext(RBRACK)) {
    after = afterType.Advance().Advance();
  } else {
    Result<Expr> nested;
    Result<Token> rbrack;

    Parser fullAfter = afterType
      .Advance() // LBRACK.
      .ParseExpression(&nested)
      .ParseTokenIf(ExactType(RBRACK), &rbrack);
    if (!fullAfter) {
      // Collect the first error, and use that.
      ErrorList errors;
      FirstOf(&errors, &nested, &rbrack);
      return Fail(move(errors), out);
    }

    sizeExpr = nested.Release();
    after = fullAfter;
  }

  Expr* newExpr = new NewArrayExpr(type.Release(), sizeExpr);
  Result<Expr> nested;
  Parser afterEnd = after.ParsePrimaryEndNoArrayAccess(newExpr, &nested);
  RETURN_IF_GOOD(afterEnd, nested.Release(), out);

  return after.Success(newExpr, out);
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

  if (IsNext(LPAREN)) {
    Result<Token> lparen;
    Result<Expr> expr;
    Result<Token> rparen;

    Parser after = (*this)
      .ParseTokenIf(ExactType(LPAREN), &lparen)
      .ParseExpression(&expr)
      .ParseTokenIf(ExactType(RPAREN), &rparen);
    RETURN_IF_GOOD(after, expr.Release(), out);

    ErrorList errors;
    FirstOf(&errors, &lparen, &expr, &rparen);
    return Fail(move(errors), out);
  }

  if (IsNext(IDENTIFIER)) {
    Result<QualifiedName> name;
    Parser after = ParseQualifiedName(&name);
    RETURN_IF_GOOD(after, new NameExpr(name.Release()), out);

    *out = ConvertError<QualifiedName, Expr>(move(name));
    return Fail();
  }

  return Fail(MakeUnexpectedTokenError(GetNext()), out);
}

Parser Parser::ParsePrimaryEnd(Expr* base, Result<Expr>* out) const {
  // PrimaryEnd:
  //   "[" Expression "]" [ PrimaryEndNoArrayAccess ]
  //   PrimaryEndNoArrayAccess
  SHORT_CIRCUIT;

  if (IsNext(LBRACK)) {
    Result<Token> lbrack;
    Result<Expr> expr;
    Result<Token> rbrack;

    Parser after = (*this)
      .ParseTokenIf(ExactType(LBRACK), &lbrack)
      .ParseExpression(&expr)
      .ParseTokenIf(ExactType(RBRACK), &rbrack);

    if (!after) {
      ErrorList errors;
      FirstOf(&errors, &lbrack, &expr, &rbrack);
      return Fail(move(errors), out);
    }

    // Try optional PrimaryEndNoArrayAccess.
    Expr* index = new ArrayIndexExpr(base, expr.Release());
    Result<Expr> nested;
    Parser afterEnd = after.ParsePrimaryEndNoArrayAccess(index, &nested);
    RETURN_IF_GOOD(afterEnd, nested.Release(), out);

    // If it failed, return what we had so far.
    return after.Success(index, out);
  }

  return ParsePrimaryEndNoArrayAccess(base, out);
}

Parser Parser::ParsePrimaryEndNoArrayAccess(Expr* base, Result<Expr>* out) const {
  // PrimaryEndNoArrayAccess:
  //   "." Identifier [ PrimaryEnd ]
  //   "(" [ArgumentList] ")" [ PrimaryEnd ]
  SHORT_CIRCUIT;

  if (IsAtEnd()) {
    return Fail(MakeUnexpectedEOFError(), out);
  }

  if (!IsNext(DOT) && !IsNext(LPAREN)) {
    return Fail(MakeUnexpectedTokenError(GetNext()), out);
  }

  if (IsNext(DOT)) {
    Result<Token> dot;
    Result<Token> ident;
    Parser after = (*this)
      .ParseTokenIf(ExactType(DOT), &dot)
      .ParseTokenIf(ExactType(IDENTIFIER), &ident);

    if (!after) {
      ErrorList errors;
      FirstOf(&errors, &dot, &ident);
      return Fail(move(errors), out);
    }

    Expr* deref = new FieldDerefExpr(base, TokenString(GetFile(), *ident.Get()), *ident.Get());
    Result<Expr> nested;
    Parser afterEnd = after.ParsePrimaryEnd(deref, &nested);
    RETURN_IF_GOOD(afterEnd, nested.Release(), out);

    return after.Success(deref, out);
  }

  {
    Result<Token> lparen;
    Result<ArgumentList> args;
    Result<Token> rparen;

    Parser after = (*this)
      .ParseTokenIf(ExactType(LPAREN), &lparen)
      .ParseArgumentList(&args)
      .ParseTokenIf(ExactType(RPAREN), &rparen);

    if (!after) {
      ErrorList errors;
      FirstOf(&errors, &lparen, &args, &rparen);
      return Fail(move(errors), out);
    }

    Expr* call = new CallExpr(base, *lparen.Get(), args.Release());
    Result<Expr> nested;
    Parser afterEnd = after.ParsePrimaryEnd(call, &nested);
    RETURN_IF_GOOD(afterEnd, nested.Release(), out);

    return after.Success(call, out);
  }
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

  while (cur.IsNext(COMMA)) {
    Result<Token> comma;
    Result<Expr> expr;

    Parser next = cur.ParseTokenIf(ExactType(COMMA), &comma).ParseExpression(&expr);
    if (!next) {
        // Fail on hanging comma.
        ErrorList errors;
        FirstOf(&errors, &comma, &expr);
        return Fail(move(errors), out);
    }

    args.Append(expr.Release());
    cur = next;
  }

  return cur.Success(new ArgumentList(move(args)), out);
}

Parser Parser::ParseStmt(Result<Stmt>* out) const {
  // Statement:
  //   ";"
  //   Block
  //   ReturnStatement
  //   IfStatement
  //   ForStatement
  //   Expression ";"
  SHORT_CIRCUIT;

  if (IsNext(SEMI)) {
    return Advance().Success(new EmptyStmt(), out);
  }

  if (IsNext(LBRACE)) {
    return ParseBlock(out);
  }

  if (IsNext(K_RETURN)) {
    return ParseReturnStmt(out);
  }

  if (IsNext(K_IF)) {
    return ParseIfStmt(out);
  }

  if (IsNext(K_FOR)) {
    return ParseForStmt(out);
  }

  {
    Result<Expr> expr;
    Result<Token> semi;
    Parser after = (*this)
      .ParseExpression(&expr)
      .ParseTokenIf(ExactType(SEMI), &semi);
    RETURN_IF_GOOD(after, new ExprStmt(expr.Release()), out);

    // Fail on last case.
    ErrorList errors;
    FirstOf(&errors, &expr, &semi);
    return Fail(move(errors), out);
  }
}

Parser Parser::ParseVarDecl(Result<Stmt>* out) const {
  // LocalVariableDeclaration:
  //   Type Identifier "=" Expression
  SHORT_CIRCUIT;

  Result<Type> type;
  Result<Token> ident;
  Result<Token> eq;
  Result<Expr> expr;
  Parser after = (*this)
    .ParseType(&type)
    .ParseTokenIf(ExactType(IDENTIFIER), &ident)
    .ParseTokenIf(ExactType(ASSG), &eq)
    .ParseExpression(&expr);
  RETURN_IF_GOOD(
      after,
      new LocalDeclStmt(
        type.Release(), *ident.Get(), expr.Release()),
      out);

  // TODO: Make it fatal error only after we find equals?
  ErrorList errors;
  FirstOf(&errors, &type, &ident, &eq, &expr);
  return Fail(move(errors), out);
}

Parser Parser::ParseReturnStmt(Result<Stmt>* out) const {
  // ReturnStatement:
  //  "return" [Expression] ";"
  SHORT_CIRCUIT;

  Result<Token> ret;
  Parser afterRet = ParseTokenIf(ExactType(K_RETURN), &ret);

  if (afterRet && afterRet.IsNext(SEMI)) {
    return afterRet.Advance().Success(new ReturnStmt(nullptr), out);
  }

  Result<Expr> expr;
  Result<Token> semi;
  Parser afterAll = afterRet
    .ParseExpression(&expr)
    .ParseTokenIf(ExactType(SEMI), &semi);

  RETURN_IF_GOOD(
      afterAll,
      new ReturnStmt(expr.Release()),
      out);

  ErrorList errors;
  FirstOf(&errors, &ret, &expr, &semi);
  return Fail(move(errors), out);
}

Parser Parser::ParseBlock(Result<Stmt>* out) const {
  // Block:
  //   "{" {BlockStatement} "}"
  // BlockStatement:
  //   LocalVariableDeclaration ";"
  //   Statement
  SHORT_CIRCUIT;

  UniquePtrVector<Stmt> stmts;
  if (IsAtEnd()) {
    return Fail(MakeUnexpectedEOFError(), out);
  }
  if (!IsNext(LBRACE)) {
    return Fail(MakeUnexpectedTokenError(GetNext()), out);
  }

  Parser cur = Advance();
  while (!cur.IsNext(RBRACE)) {
    {
      Result<Stmt> varDecl;
      Result<Token> semi;
      Parser next = cur
        .ParseVarDecl(&varDecl)
        .ParseTokenIf(ExactType(SEMI), &semi);
      if (next) {
        stmts.Append(varDecl.Release());
        cur = next;
        continue;
      }
    }

    {
      Result<Stmt> stmt;
      Parser next = cur.ParseStmt(&stmt);
      if (next) {
        stmts.Append(stmt.Release());
        cur = next;
        continue;
      }
      ErrorList errors;
      stmt.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }
  }

  return cur.Advance().Success(new BlockStmt(move(stmts)), out);
}

Parser Parser::ParseIfStmt(Result<Stmt>* out) const {
  // IfStatement:
  //   "if" "(" Expression ")" Statement ["else" Statement]
  SHORT_CIRCUIT;

  Result<Token> tokIf;
  Result<Token> lparen;
  Result<Expr> expr;
  Result<Token> rparen;
  Result<Stmt> stmt;

  Parser after = (*this)
    .ParseTokenIf(ExactType(K_IF), &tokIf)
    .ParseTokenIf(ExactType(LPAREN), &lparen)
    .ParseExpression(&expr)
    .ParseTokenIf(ExactType(RPAREN), &rparen)
    .ParseStmt(&stmt);
  if (!after) {
    ErrorList errors;
    FirstOf(&errors, &tokIf, &lparen, &expr, &rparen, &stmt);
    return Fail(move(errors), out);
  }

  if (!after.IsNext(K_ELSE)) {
    return after.Success(new IfStmt(expr.Release(), stmt.Release(), new EmptyStmt()), out);
  }

  Result<Stmt> elseStmt;
  Parser afterElse = after.Advance().ParseStmt(&elseStmt);
  RETURN_IF_GOOD(
      afterElse,
      new IfStmt(expr.Release(), stmt.Release(), elseStmt.Release()),
      out);

  // Committed to having else, so fail.
  ErrorList errors;
  elseStmt.ReleaseErrors(&errors);
  return Fail(move(errors), out);
}

Parser Parser::ParseForInit(Result<Stmt>* out) const {
  // ForInit:
  //   LocalVariableDeclaration
  //   Expression
  SHORT_CIRCUIT;

  {
    Result<Stmt> varDecl;
    Parser after = ParseVarDecl(&varDecl);
    RETURN_IF_GOOD(after, varDecl.Release(), out);
  }

  {
    Result<Expr> expr;
    Parser after = ParseExpression(&expr);
    // Note: This ExprStmt didn't consume a semicolon!
    RETURN_IF_GOOD(after, new ExprStmt(expr.Release()), out);
    ErrorList errors;
    expr.ReleaseErrors(&errors);
    return Fail(move(errors), out);
  }
}

Parser Parser::ParseForStmt(Result<Stmt>* out) const {
  // ForStatement:
  //   "for" "(" [ForInit] ";" [Expression] ";" [ForUpdate] ")" Statement
  SHORT_CIRCUIT;

  Result<Token> forTok;
  Result<Token> lparen;
  Parser next = (*this)
    .ParseTokenIf(ExactType(K_FOR), &forTok)
    .ParseTokenIf(ExactType(LPAREN), &lparen);

  if (!next) {
    ErrorList errors;
    FirstOf(&errors, &forTok, &lparen);
    return Fail(move(errors), out);
  }

  // TODO: Make emptystmt not print anything.

  // Parse optional for initializer.
  unique_ptr<Stmt> forInit;
  if (next.IsNext(SEMI)) {
    forInit.reset(new EmptyStmt());
    next = next.Advance();
  } else {
    Result<Stmt> stmt;
    Result<Token> semi;
    Parser afterInit = next
      .ParseForInit(&stmt)
      .ParseTokenIf(ExactType(SEMI), &semi);
    if (!afterInit) {
      ErrorList errors;
      FirstOf(&errors, &stmt, &semi);
      return next.Fail(move(errors), out);
    }
    forInit.reset(stmt.Release());
    next = afterInit;
  }

  // Parse optional for condition.
  unique_ptr<Expr> forCond = nullptr;
  if (next.IsNext(SEMI)) {
    next = next.Advance();
  } else {
    Result<Expr> cond;
    Result<Token> semi;
    Parser afterCond = next
      .ParseExpression(&cond)
      .ParseTokenIf(ExactType(SEMI), &semi);
    if (!afterCond) {
      ErrorList errors;
      FirstOf(&errors, &cond, &semi);
      return next.Fail(move(errors), out);
    }
    forCond.reset(cond.Release());
    next = afterCond;
  }

  // Parse optional for update.
  unique_ptr<Expr> forUpdate;
  if (!next.IsNext(RPAREN)) {
    Result<Expr> update;
    Parser afterUpdate = next.ParseExpression(&update);
    if (!afterUpdate) {
      ErrorList errors;
      update.ReleaseErrors(&errors);
      return next.Fail(move(errors), out);
    }
    forUpdate.reset(update.Release());
    next = afterUpdate;
  }

  // Parse RParen and statement.
  Result<Token> rparen;
  Result<Stmt> body;
  Parser after = next
    .ParseTokenIf(ExactType(RPAREN), &rparen)
    .ParseStmt(&body);
  RETURN_IF_GOOD(after, new ForStmt(
        forInit.release(), forCond.release(), forUpdate.release(), body.Release()), out);

  ErrorList errors;
  FirstOf(&errors, &rparen, &body);
  return next.Fail(move(errors), out);
}

Parser Parser::ParseModifierList(Result<ModifierList>* out) const {
  // ModifierList:
  //   {Modifier}
  // Modifier:
  //   public
  //   protected
  //   abstract
  //   final
  //   static
  //   native
  SHORT_CIRCUIT;

  ModifierList ml;
  Parser cur = *this;
  while (true) {
    Result<Token> tok;
    Parser next = cur.ParseTokenIf(IsModifier(), &tok);
    if (!next) {
      return cur.Success(new ModifierList(move(ml)), out);
    }
    if (!ml.AddModifier(*tok.Get())) {
      return cur.Fail(MakeDuplicateModifierError(*tok.Get()), out);
    }
    cur = next;
  }
}

Parser Parser::ParseMemberDecl(Result<MemberDecl>* out) const {
  // MethodOrFieldDecl:
  //   ModifierList Type Identifier MethodOrFieldDeclEnd
  // MethodOrFieldDeclEnd:
  //   "(" [FormalParameterList] ")" MethodBody
  //   ["=" Expression] ";"
  // MethodBody:
  //   Block
  //   ";"
  SHORT_CIRCUIT;

  Result<ModifierList> mods;
  Result<Type> type;
  Result<Token> ident;
  Parser afterCommon = (*this)
    .ParseModifierList(&mods)
    .ParseType(&type)
    .ParseTokenIf(ExactType(IDENTIFIER), &ident);
  if (!afterCommon) {
    ErrorList errors;
    FirstOf(&errors, &mods, &type, &ident);
    return Fail(move(errors), out);
  }

  // Parse method.
  if (afterCommon.IsNext(LPAREN)) {
    Result<ParamList> params;
    Result<Token> rparen;
    Parser afterParams = afterCommon.Advance()
      .ParseParamList(&params)
      .ParseTokenIf(ExactType(RPAREN), &rparen);

    if (!afterParams) {
      ErrorList errors;
      FirstOf(&errors, &params, &rparen);
      return afterCommon.Fail(move(errors), out);
    }

    unique_ptr<Stmt> bodyPtr(nullptr);
    Parser afterBody = afterParams;
    if (afterParams.IsNext(SEMI)) {
      bodyPtr.reset(new EmptyStmt());
      afterBody = afterParams.Advance();
    } else {
      Result<Stmt> body;
      afterBody = afterParams.ParseBlock(&body);
      if (!afterBody) {
        ErrorList errors;
        body.ReleaseErrors(&errors);
        return afterParams.Fail(move(errors), out);
      }
      bodyPtr.reset(body.Release());
    }

    return afterBody.Success(
        new MethodDecl(std::move(*mods.Get()), type.Release(), *ident.Get(), std::move(*params.Get()), bodyPtr.release()),
        out);
  }

  // Parse field.
  if (afterCommon.IsNext(SEMI)) {
    return afterCommon.Advance().Success(
        new FieldDecl(std::move(*mods.Get()), type.Release(), *ident.Get(), nullptr), out);
  }

  Result<Token> eq;
  Result<Expr> val;
  Result<Token> semi;
  Parser afterVal = afterCommon
    .ParseTokenIf(ExactType(ASSG), &eq)
    .ParseExpression(&val)
    .ParseTokenIf(ExactType(SEMI), &semi);

  RETURN_IF_GOOD(
      afterVal,
      new FieldDecl(std::move(*mods.Get()), type.Release(), *ident.Get(), val.Release()),
      out);

  ErrorList errors;
  FirstOf(&errors, &eq, &val, &semi);
  return afterCommon.Fail(move(errors), out);
}

Parser Parser::ParseParamList(Result<ParamList>* out) const {
  // FormalParameterList:
  //   [FormalParameter {, FormalParameter}]
  // FormalParameter:
  //   Type Identifier
  SHORT_CIRCUIT;

  UniquePtrVector<Param> params;
  Parser cur = *this;
  bool firstParam = true;
  while (true) {
    Result<Type> type;
    Result<Token> ident;
    Parser afterType = cur.ParseType(&type);
    if (!afterType) {
      // If can't parse type then return empty param list.
      if (firstParam) {
        break;
      }
      // Bad token or EOF after a comma.
      ErrorList errors;
      type.ReleaseErrors(&errors);
      return cur.Fail(move(errors), out);
    }
    firstParam = false;

    Parser afterIdent = afterType
      .ParseTokenIf(ExactType(IDENTIFIER), &ident);
    if (!afterIdent) {
      // Committed to getting identifier.
      return afterType.Fail(MakeParamRequiresNameError(cur.GetNext()), out);
    }
    cur = afterIdent;
    params.Append(new Param(type.Release(), *ident.Get()));

    if (cur.IsNext(COMMA)) {
      cur = cur.Advance();
    } else {
      break;
    }
  }
  return cur.Success(new ParamList(move(params)), out);
}

Parser Parser::ParseTypeDecl(Result<TypeDecl>* out) const {
  //TypeDeclaration:
  //  ClassDeclaration
  //  InterfaceDeclaration
  //  ";"
  //ClassDeclaration:
  //  ModifierList "class" Identifier ["extends" QualifiedName] ["implements" Interfaces] ClassBody
  //InterfaceDeclaration:
  //  ModifierList "interface" Identifier ["extends" Interfaces] ClassBody
  //Interfaces:
  //  QualifiedName {"," QualifiedName}

  SHORT_CIRCUIT;

  Result<ModifierList> mods;
  Parser afterMods = (*this)
    .ParseModifierList(&mods);
  if (!afterMods) {
    ErrorList errors;
    mods.ReleaseErrors(&errors);
    return Fail(move(errors), out);
  }

  if (IsAtEnd()) {
    return afterMods.Fail(MakeUnexpectedEOFError(), out);
  }

  if (!afterMods.IsNext(K_CLASS) && !afterMods.IsNext(K_INTERFACE)) {
    return Fail(MakeUnexpectedTokenError(afterMods.GetNext()), out);
  }

  bool isClass = true;
  Result<Token> typeToken;

  Parser afterType = afterMods.ParseTokenIf(ExactType(K_CLASS), &typeToken);
  if (!afterType) {
    afterType = afterMods.ParseTokenIf(ExactType(K_INTERFACE), &typeToken);
    isClass = false;
  }

  Result<Token> ident;
  Parser afterIdent = afterType.ParseTokenIf(ExactType(IDENTIFIER), &ident);

  if (!afterIdent) {
    ErrorList errors;
    ident.ReleaseErrors(&errors);
    return Fail(move(errors), out);
  }

  Parser afterSuper = afterIdent;
  unique_ptr<ReferenceType> super(nullptr);
  if (isClass && afterIdent.IsNext(K_EXTENDS)) {
    Result<QualifiedName> superName;

    afterSuper = afterIdent
      .Advance()
      .ParseQualifiedName(&superName);

    if (!afterSuper) {
      ErrorList errors;
      superName.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }

    super.reset(new ReferenceType(superName.Release()));
  }

  Parser afterInterfaces = afterSuper;
  UniquePtrVector<ReferenceType> interfaces;

  if ((isClass && afterSuper.IsNext(K_IMPLEMENTS)) || afterSuper.IsNext(K_EXTENDS)) {
    afterInterfaces = afterInterfaces.Advance();

    while (true) {
      Result<QualifiedName> name;
      Parser afterName = afterInterfaces.ParseQualifiedName(&name);
      if (!afterName) {
        // Bad token or EOF after a comma.
        ErrorList errors;
        name.ReleaseErrors(&errors);
        return afterInterfaces.Fail(move(errors), out);
      }

      afterInterfaces = afterName;
      interfaces.Append(new ReferenceType(name.Release()));

      if (afterInterfaces.IsNext(COMMA)) {
        afterInterfaces = afterInterfaces.Advance();
      } else {
        break;
      }
    }
  }

  Result<Token> lbrace;
  Parser afterBrace = afterInterfaces.ParseTokenIf(ExactType(LBRACE), &lbrace);
  if (!afterBrace) {
    ErrorList errors;
    lbrace.ReleaseErrors(&errors);
    return afterInterfaces.Fail(move(errors), out);
  }

  UniquePtrVector<MemberDecl> members;

  Parser afterBody = afterBrace;
  while (!afterBody.IsNext(RBRACE)) {
    if (afterBody.IsNext(SEMI)) {
      afterBody = afterBody.Advance();
      continue;
    }

    Result<MemberDecl> member;
    Parser afterMember = afterBody.ParseMemberDecl(&member);

    if (!afterMember) {
      ErrorList errors;
      member.ReleaseErrors(&errors);
      return afterBody.Fail(move(errors), out);
    }

    members.Append(member.Release());
    afterBody = afterMember;
  }

  TypeDecl* type = nullptr;

  if (isClass) {
    type = new ClassDecl(move(*mods.Release()), *ident.Get(), move(interfaces), move(members), super.release());
  } else {
    type = new InterfaceDecl(move(*mods.Release()), *ident.Get(), move(interfaces), move(members));
  }

  return afterBody.Advance().Success(type, out);
}

// TODO: move Weed function to weeder package.
void Weed(const FileSet* fs, TypeDecl* ast, ErrorList* out) {
  weeder::AssignmentVisitor assignmentChecker(fs, out);
  ast->Accept(&assignmentChecker);

  weeder::CallVisitor callChecker(fs, out);
  ast->Accept(&callChecker);

  weeder::TypeVisitor typeChecker(fs, out);
  ast->Accept(&typeChecker);

  weeder::ModifierVisitor modifierChecker(fs, out);
  ast->Accept(&modifierChecker);

  // More weeding required.
}

void Parse(const FileSet* fs, const File* file, const vector<Token>* tokens) {
  Parser parser(fs, file, tokens, 0);
  Result<TypeDecl> result;
  parser.ParseTypeDecl(&result);
  if (!result.IsSuccess()) {
    result.Errors().PrintTo(&std::cout, base::OutputOptions::kUserOutput);
    return;
  }

  ErrorList errors;
  Weed(fs, result.Get(), &errors);
  if (errors.IsFatal()) {
    errors.PrintTo(&std::cout, base::OutputOptions::kUserOutput);
    return;
  }

  PrintVisitor printer = PrintVisitor::Pretty(&std::cout);
  result.Get()->Accept(&printer);
  std::cout << '\n';
}

// TODO: Compilation unit should check for ";" in place of a TypeDeclaration.
// TODO: in for-loop initializers, for-loop incrementors, and top-level
// statements, we must ensure that they are either assignment, method
// invocation, or class creation, not other types of expressions (like
// boolean ops).
// TODO: Weed out statements of the form "new PrimitiveType([ArgumentList])".
// TODO: Handle parsing empty strings.
// TODO: Weed expressions of the form "f()()()()...".
// TODO: Weed out void primitive type for all but method decls.
// TODO: Weed: interface can't contain constructor.
// TODO: Weed: class can't contain abstract method decls.

} // namespace parser
