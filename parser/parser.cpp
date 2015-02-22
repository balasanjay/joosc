#include "parser/parser.h"

#include "ast/ast.h"
#include "base/unique_ptr_vector.h"
#include "lexer/lexer.h"
#include "parser/parser_internal.h"

using namespace ast;

using std::cerr;
using std::function;
using std::move;

using base::Error;
using base::ErrorList;
using base::File;
using base::FileSet;
using base::Pos;
using base::UniquePtrVector;
using base::SharedPtrVector;
using lexer::ADD;
using lexer::ASSG;
using lexer::CHAR;
using lexer::COMMA;
using lexer::DOT;
using lexer::IDENTIFIER;
using lexer::INTEGER;
using lexer::K_CLASS;
using lexer::K_ELSE;
using lexer::K_EXTENDS;
using lexer::K_FALSE;
using lexer::K_FOR;
using lexer::K_IF;
using lexer::K_IMPLEMENTS;
using lexer::K_IMPORT;
using lexer::K_INTERFACE;
using lexer::K_NEW;
using lexer::K_NULL;
using lexer::K_PACKAGE;
using lexer::K_RETURN;
using lexer::K_THIS;
using lexer::K_TRUE;
using lexer::K_WHILE;
using lexer::LBRACE;
using lexer::LBRACK;
using lexer::LPAREN;
using lexer::MUL;
using lexer::RBRACE;
using lexer::RBRACK;
using lexer::RPAREN;
using lexer::SEMI;
using lexer::STRING;
using lexer::Token;
using lexer::TokenType;
using parser::internal::ConvertError;
using parser::internal::Result;

#define RETURN_IF_ERR(check)    \
  {                             \
    if (!(check).IsSuccess()) { \
      return (check);           \
    }                           \
  }

#define RETURN_IF_GOOD(parser, value, out)                           \
  {                                                                  \
    Parser _hope_you_didnt_declare_this = (parser);                  \
    if ((_hope_you_didnt_declare_this)) {                            \
      return (_hope_you_didnt_declare_this).Success((value), (out)); \
    }                                                                \
  }

#define SHORT_CIRCUIT \
  {                   \
    if (!(*this)) {   \
      return *this;   \
    }                 \
  };                  \

namespace parser {

namespace {

// Predicates for ParseTokenIf.
struct ExactType {
  ExactType(TokenType type) : type_(type) {}
  bool operator()(const Token& t) { return t.type == type_; }

 private:
  TokenType type_;
};

#define TYPE_INFO_PRED(expr) auto expr = [](const Token& t) { return t.TypeInfo().expr(); }
TYPE_INFO_PRED(IsBinOp);
TYPE_INFO_PRED(IsUnaryOp);
TYPE_INFO_PRED(IsLiteral);
TYPE_INFO_PRED(IsPrimitive);
TYPE_INFO_PRED(IsModifier);
#undef TYPE_INFO_PRED

sptr<const Expr> FixPrecedence(const SharedPtrVector<const Expr>& owned_exprs,
                    const vector<Token>& ops) {
  vector<sptr<const Expr>> outstack;
  vector<Token> opstack;

  vector<sptr<const Expr>> exprs;
  for (int i = 0; i < owned_exprs.Size(); ++i) {
    exprs.push_back(owned_exprs.At(i));
  }

  assert(exprs.size() == ops.size() + 1);

  uint i = 0;
  while (i < exprs.size() + ops.size() || opstack.size() > 0) {
    if (i < exprs.size() + ops.size()) {
      if (i % 2 == 0) {
        // Expr off input.
        sptr<const Expr> e = exprs.at(i / 2);
        outstack.push_back(e);
        i++;
        continue;
      }

      // Op off input.
      assert(i % 2 == 1);
      Token op = ops.at(i / 2);

      if (opstack.empty() || (op.type == ASSG &&
                              op.TypeInfo().BinOpPrec() >=
                                  opstack.rbegin()->TypeInfo().BinOpPrec()) ||
          (op.type != ASSG &&
           op.TypeInfo().BinOpPrec() >
               opstack.rbegin()->TypeInfo().BinOpPrec())) {
        opstack.push_back(op);
        i++;
        continue;
      }
    }

    assert(outstack.size() >= 2);

    sptr<const Expr> rhs = *outstack.rbegin();
    sptr<const Expr> lhs = *(outstack.rbegin() + 1);
    Token nextop = *opstack.rbegin();

    outstack.pop_back();
    outstack.pop_back();
    opstack.pop_back();

    outstack.push_back(make_shared<BinExpr>(lhs, nextop, rhs));
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

QualifiedName MakeQualifiedName(const File* file,
                                const vector<Token>& tokens) {
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

  return QualifiedName(tokens, parts, fullname.str());
}

}  // namespace

Parser Parser::EatSemis() const {
  SHORT_CIRCUIT;
  Parser cur = *this;
  while (cur.IsNext(SEMI)) {
    cur = cur.Advance();
  }
  return cur;
}

Error* Parser::MakeUnexpectedTokenError(Token token) const {
  // TODO: say what you expected instead.
  return MakeSimplePosRangeError(fs_, Pos(token.pos.fileid, token.pos.begin),
                                 "UnexpectedTokenError", "Unexpected token.");
}

Error* Parser::MakeDuplicateModifierError(Token token) const {
  return MakeSimplePosRangeError(fs_, token.pos, "DuplicateModifierError",
                                 "Duplicate modifier.");
}

Error* Parser::MakeParamRequiresNameError(Token token) const {
  return MakeSimplePosRangeError(fs_, token.pos, "ParamRequiresNameError",
                                 "A parameter requires a type and a name.");
}

Error* Parser::MakeUnexpectedEOFError() const {
  // TODO: say what you expected instead.
  // TODO: this will crash on an empty file.
  return MakeSimplePosRangeError(
      fs_, Pos(tokens_->at(0).pos.fileid, file_->Size() - 1),
      "UnexpectedEOFError", "Unexpected end-of-file.");
}

Parser Parser::ParseTokenIf(function<bool(Token)> pred,
                            Result<Token>* out) const {
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
    Parser next = cur.ParseTokenIf(ExactType(DOT), &dot)
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
  QualifiedName result = MakeQualifiedName(GetFile(), tokens);
  return cur.Success(new QualifiedName(result), out);
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
  Parser after = ParseTokenIf(IsPrimitive, &primitive);
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
    RETURN_IF_GOOD(after, new ReferenceType(*reference.Get()), out);

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

    Parser afterArray = afterSingle.ParseTokenIf(ExactType(LBRACK), &lbrack)
                            .ParseTokenIf(ExactType(RBRACK), &rbrack);
    RETURN_IF_GOOD(afterArray, new ArrayType(single.Get(), *lbrack.Get(), *rbrack.Get()), out);

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

  SharedPtrVector<const Expr> exprs;
  vector<Token> operators;

  Result<Expr> expr;
  Parser cur = ParseUnaryExpression(&expr);
  if (!cur) {
    *out = move(expr);
    return cur;
  }

  exprs.Append(expr.Get());

  while (cur.IsNext(IsBinOp)) {
    Result<Token> binOp;
    Result<Expr> nextExpr;

    Parser next = cur.ParseTokenIf(IsBinOp, &binOp);
    if (!next) {
      ErrorList errors;
      binOp.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }

    // Check if binop is instanceof.
    if (binOp.Get()->type == lexer::K_INSTANCEOF) {
      Result<Type> instanceOfType;
      next = next.ParseType(&instanceOfType);
      if (!next) {
        ErrorList errors;
        instanceOfType.ReleaseErrors(&errors);
        return Fail(move(errors), out);
      }
      sptr<const Expr> instanceOfLhs = exprs.PopBack();
      exprs.Append(make_shared<InstanceOfExpr>(instanceOfLhs, *binOp.Get(),
                                      instanceOfType.Get()));
    } else {
      next = next.ParseUnaryExpression(&nextExpr);
      if (!next) {
        ErrorList errors;
        FirstOf(&errors, &binOp, &nextExpr);
        return Fail(move(errors), out);
      }

      operators.push_back(*binOp.Get());
      exprs.Append(nextExpr.Get());
    }
    cur = next;
  }

  return cur.Success(FixPrecedence(exprs, operators), out);
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

  if (IsNext(IsUnaryOp)) {
    Result<Token> unaryOp;
    Result<Expr> expr;
    Parser after =
        ParseTokenIf(IsUnaryOp, &unaryOp).ParseUnaryExpression(&expr);
    RETURN_IF_GOOD(after, new UnaryExpr(*unaryOp.Get(), expr.Get()), out);

    ErrorList errors;
    FirstOf(&errors, &unaryOp, &expr);
    return Fail(move(errors), out);
  }

  {
    Result<Expr> expr;
    Parser after = ParseCastExpression(&expr);
    RETURN_IF_GOOD(after, expr.Get(), out);
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
  RETURN_IF_GOOD(after, new CastExpr(*lparen.Get(), type.Get(), *rparen.Get(), expr.Get()), out);

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
  sptr<const Expr> baseExpr = base.Get();
  Result<Expr> baseWithEnds;
  Parser afterEnds = afterBase.ParsePrimaryEnd(baseExpr, &baseWithEnds);
  RETURN_IF_GOOD(afterEnds, baseWithEnds.Get(), out);

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
  Parser afterType =
      ParseTokenIf(ExactType(K_NEW), &newTok).ParseSingleType(&type);
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
    Result<SharedPtrVector<const Expr>> args;
    Result<Token> rparen;
    Parser afterCall = afterType.ParseTokenIf(ExactType(LPAREN), &lparen)
                           .ParseArgumentList(&args)
                           .ParseTokenIf(ExactType(RPAREN), &rparen);

    if (!afterCall) {
      // Collect the first error, and use that.
      ErrorList errors;
      FirstOf(&errors, &lparen, &args, &rparen);
      return Fail(move(errors), out);
    }

    sptr<const Expr> newExpr =
        make_shared<NewClassExpr>(*newTok.Get(), type.Get(), *lparen.Get(), *args.Get(), *rparen.Get());
    Result<Expr> nested;
    Parser afterEnd = afterCall.ParsePrimaryEnd(newExpr, &nested);
    RETURN_IF_GOOD(afterEnd, nested.Get(), out);

    return afterCall.Success(newExpr, out);
  }

  assert(afterType.IsNext(LBRACK));
  Result<Token> lbrack;
  sptr<const Expr> sizeExpr = nullptr;
  Result<Token> rbrack;
  Parser after = *this;

  if (afterType.Advance().IsNext(RBRACK)) {
    after = afterType
      .ParseTokenIf(ExactType(LBRACK), &lbrack)
      .ParseTokenIf(ExactType(RBRACK), &rbrack);
    after = afterType.Advance().Advance();
  } else {
    Result<Expr> nested;

    Parser fullAfter = afterType
                           .ParseTokenIf(ExactType(LBRACK), &lbrack)
                           .ParseExpression(&nested)
                           .ParseTokenIf(ExactType(RBRACK), &rbrack);
    if (!fullAfter) {
      // Collect the first error, and use that.
      ErrorList errors;
      FirstOf(&errors, &nested, &rbrack);
      return Fail(move(errors), out);
    }

    sizeExpr = nested.Get();
    after = fullAfter;
  }

  sptr<const Expr> newExpr = make_shared<NewArrayExpr>(*newTok.Get(), type.Get(), *lbrack.Get(), sizeExpr, *rbrack.Get());
  Result<Expr> nested;
  Parser afterEnd = after.ParsePrimaryEndNoArrayAccess(newExpr, &nested);
  RETURN_IF_GOOD(afterEnd, nested.Get(), out);

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

  if (IsNext(IsLiteral)) {
    Token lit = GetNext();
    Parser after = (*this).Advance();
    switch (lit.type) {
      case INTEGER:
        return after.Success(new IntLitExpr(lit, TokenString(GetFile(), lit)),
                             out);
      case CHAR:
        return after.Success(new CharLitExpr(lit), out);
      case K_TRUE:
      case K_FALSE:
        return after.Success(new BoolLitExpr(lit), out);
      case K_NULL:
        return after.Success(new NullLitExpr(lit), out);
      case STRING:
        return after.Success(new StringLitExpr(lit), out);
      default:
        throw;
    }
  }

  {
    Result<Token> thisTok;
    Parser after = ParseTokenIf(ExactType(K_THIS), &thisTok);
    RETURN_IF_GOOD(after, new ThisExpr(*thisTok.Get()), out);
  }

  if (IsNext(LPAREN)) {
    Result<Token> lparen;
    Result<Expr> expr;
    Result<Token> rparen;

    Parser after = (*this)
                       .ParseTokenIf(ExactType(LPAREN), &lparen)
                       .ParseExpression(&expr)
                       .ParseTokenIf(ExactType(RPAREN), &rparen);
    RETURN_IF_GOOD(after, new ParenExpr(*lparen.Get(), expr.Get(), *rparen.Get()), out);

    ErrorList errors;
    FirstOf(&errors, &lparen, &expr, &rparen);
    return Fail(move(errors), out);
  }

  if (IsNext(IDENTIFIER)) {
    Result<QualifiedName> name;
    Parser after = ParseQualifiedName(&name);
    RETURN_IF_GOOD(after, new NameExpr(*name.Get()), out);

    *out = ConvertError<QualifiedName, Expr>(move(name));
    return Fail();
  }

  return Fail(MakeUnexpectedTokenError(GetNext()), out);
}

Parser Parser::ParsePrimaryEnd(sptr<const Expr> base, Result<Expr>* out) const {
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
    sptr<const Expr> index = make_shared<ArrayIndexExpr>(base, *lbrack.Get(), expr.Get(), *rbrack.Get());
    Result<Expr> nested;
    Parser afterEnd = after.ParsePrimaryEndNoArrayAccess(index, &nested);
    RETURN_IF_GOOD(afterEnd, nested.Get(), out);

    // If it failed, return what we had so far.
    return after.Success(index, out);
  }

  return ParsePrimaryEndNoArrayAccess(base, out);
}

Parser Parser::ParsePrimaryEndNoArrayAccess(sptr<const Expr> base,
                                            Result<Expr>* out) const {
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
    Parser after = (*this).ParseTokenIf(ExactType(DOT), &dot).ParseTokenIf(
        ExactType(IDENTIFIER), &ident);

    if (!after) {
      ErrorList errors;
      FirstOf(&errors, &dot, &ident);
      return Fail(move(errors), out);
    }

    sptr<const Expr> deref = make_shared<FieldDerefExpr>(base, TokenString(GetFile(), *ident.Get()),
                                     *ident.Get());
    Result<Expr> nested;
    Parser afterEnd = after.ParsePrimaryEnd(deref, &nested);
    RETURN_IF_GOOD(afterEnd, nested.Get(), out);

    return after.Success(deref, out);
  }

  {
    Result<Token> lparen;
    Result<SharedPtrVector<const Expr>> args;
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

    sptr<const Expr> call = make_shared<CallExpr>(base, *lparen.Get(), *args.Get(), *rparen.Get());
    Result<Expr> nested;
    Parser afterEnd = after.ParsePrimaryEnd(call, &nested);
    RETURN_IF_GOOD(afterEnd, nested.Get(), out);

    return after.Success(call, out);
  }
}

Parser Parser::ParseArgumentList(Result<SharedPtrVector<const Expr>>* out) const {
  // ArgumentList:
  //   [Expression {"," Expression}]
  SHORT_CIRCUIT;

  SharedPtrVector<const Expr> args;
  Result<Expr> first;
  Parser cur = ParseExpression(&first);
  if (!cur) {
    return Success(new SharedPtrVector<const Expr>(args), out);
  }
  args.Append(first.Get());

  while (cur.IsNext(COMMA)) {
    Result<Token> comma;
    Result<Expr> expr;

    Parser next =
        cur.ParseTokenIf(ExactType(COMMA), &comma).ParseExpression(&expr);
    if (!next) {
      // Fail on hanging comma.
      ErrorList errors;
      FirstOf(&errors, &comma, &expr);
      return Fail(move(errors), out);
    }

    args.Append(expr.Get());
    cur = next;
  }

  return cur.Success(new SharedPtrVector<const Expr>(args), out);
}

Parser Parser::ParseStmt(Result<Stmt>* out) const {
  // Statement:
  //   ";"
  //   Block
  //   ReturnStatement
  //   IfStatement
  //   ForStatement
  //   WhileStatement
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

  if (IsNext(K_WHILE)) {
    return ParseWhileStmt(out);
  }

  {
    Result<Expr> expr;
    Result<Token> semi;
    Parser after =
        (*this).ParseExpression(&expr).ParseTokenIf(ExactType(SEMI), &semi);
    RETURN_IF_GOOD(after, new ExprStmt(expr.Get()), out);

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
      after, new LocalDeclStmt(type.Get(), TokenString(file_, *ident.Get()), *ident.Get(), expr.Get()),
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
  Parser afterAll =
      afterRet.ParseExpression(&expr).ParseTokenIf(ExactType(SEMI), &semi);

  RETURN_IF_GOOD(afterAll, new ReturnStmt(expr.Get()), out);

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

  SharedPtrVector<const Stmt> stmts;
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
      Parser next =
          cur.ParseVarDecl(&varDecl).ParseTokenIf(ExactType(SEMI), &semi);
      if (next) {
        stmts.Append(varDecl.Get());
        cur = next;
        continue;
      }
    }

    {
      Result<Stmt> stmt;
      Parser next = cur.ParseStmt(&stmt);
      if (next) {
        stmts.Append(stmt.Get());
        cur = next;
        continue;
      }
      ErrorList errors;
      stmt.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }
  }

  return cur.Advance().Success(new BlockStmt(stmts), out);
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
    return after.Success(
        new IfStmt(expr.Get(), stmt.Get(), make_shared<EmptyStmt>()), out);
  }

  Result<Stmt> elseStmt;
  Parser afterElse = after.Advance().ParseStmt(&elseStmt);
  RETURN_IF_GOOD(afterElse,
                 new IfStmt(expr.Get(), stmt.Get(), elseStmt.Get()),
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
    RETURN_IF_GOOD(after, varDecl.Get(), out);
  }

  {
    Result<Expr> expr;
    Parser after = ParseExpression(&expr);
    // Note: This ExprStmt didn't consume a semicolon!
    RETURN_IF_GOOD(after, new ExprStmt(expr.Get()), out);
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
  Parser next = (*this).ParseTokenIf(ExactType(K_FOR), &forTok).ParseTokenIf(
      ExactType(LPAREN), &lparen);

  if (!next) {
    ErrorList errors;
    FirstOf(&errors, &forTok, &lparen);
    return Fail(move(errors), out);
  }

  // TODO: Make emptystmt not print anything.

  // Parse optional for initializer.
  sptr<const Stmt> forInit;
  if (next.IsNext(SEMI)) {
    forInit.reset(new EmptyStmt());
    next = next.Advance();
  } else {
    Result<Stmt> stmt;
    Result<Token> semi;
    Parser afterInit =
        next.ParseForInit(&stmt).ParseTokenIf(ExactType(SEMI), &semi);
    if (!afterInit) {
      ErrorList errors;
      FirstOf(&errors, &stmt, &semi);
      return next.Fail(move(errors), out);
    }
    forInit = stmt.Get();
    next = afterInit;
  }

  // Parse optional for condition.
  sptr<const Expr> forCond = nullptr;
  if (next.IsNext(SEMI)) {
    next = next.Advance();
  } else {
    Result<Expr> cond;
    Result<Token> semi;
    Parser afterCond =
        next.ParseExpression(&cond).ParseTokenIf(ExactType(SEMI), &semi);
    if (!afterCond) {
      ErrorList errors;
      FirstOf(&errors, &cond, &semi);
      return next.Fail(move(errors), out);
    }
    forCond = cond.Get();
    next = afterCond;
  }

  // Parse optional for update.
  sptr<const Expr> forUpdate;
  if (!next.IsNext(RPAREN)) {
    Result<Expr> update;
    Parser afterUpdate = next.ParseExpression(&update);
    if (!afterUpdate) {
      ErrorList errors;
      update.ReleaseErrors(&errors);
      return next.Fail(move(errors), out);
    }
    forUpdate = update.Get();
    next = afterUpdate;
  }

  // Parse RParen and statement.
  Result<Token> rparen;
  Result<Stmt> body;
  Parser after = next.ParseTokenIf(ExactType(RPAREN), &rparen).ParseStmt(&body);
  RETURN_IF_GOOD(after, new ForStmt(forInit, forCond,
                                    forUpdate, body.Get()),
                 out);

  ErrorList errors;
  FirstOf(&errors, &rparen, &body);
  return next.Fail(move(errors), out);
}

Parser Parser::ParseWhileStmt(internal::Result<Stmt>* out) const {
  // WhileStatement:
  //   "while" "(" Expression ")" Statement

  SHORT_CIRCUIT;

  Result<Token> whileTok;
  Result<Token> lparen;
  Result<Expr> cond;
  Result<Token> rparen;
  Result<Stmt> body;

  Parser after = (*this)
                     .ParseTokenIf(ExactType(K_WHILE), &whileTok)
                     .ParseTokenIf(ExactType(LPAREN), &lparen)
                     .ParseExpression(&cond)
                     .ParseTokenIf(ExactType(RPAREN), &rparen)
                     .ParseStmt(&body);

  RETURN_IF_GOOD(after, new WhileStmt(cond.Get(), body.Get()), out);

  ErrorList errors;
  FirstOf(&errors, &whileTok, &lparen, &cond, &rparen, &body);
  return Fail(move(errors), out);
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
    Parser next = cur.ParseTokenIf(IsModifier, &tok);
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
  // ConstructorDeclaration:
  //   ModifierList ConstructorDeclarator Block
  // ConstructorDeclarator:
  //   Identifier  "(" FormalParameterList ")"
  SHORT_CIRCUIT;

  Result<ModifierList> mods;
  Parser afterMods = ParseModifierList(&mods);

  // Check if constructor - look two tokens ahead.
  bool isConstructor =
      afterMods.IsNext(IDENTIFIER) && afterMods.Advance().IsNext(LPAREN);

  Result<Type> type;
  Result<Token> ident;
  Parser afterType = afterMods;

  // Parse type if not a constructor.
  if (!isConstructor) {
    afterType = afterMods.ParseType(&type);
  }
  Parser afterCommon = afterType.ParseTokenIf(ExactType(IDENTIFIER), &ident);
  if (!afterCommon) {
    ErrorList errors;
    FirstOf(&errors, &mods, &type, &ident);
    return Fail(move(errors), out);
  }

  // Parse method.
  if (afterCommon.IsNext(LPAREN)) {
    Result<ParamList> params;
    Result<Token> rparen;
    Parser afterParams =
        afterCommon.Advance().ParseParamList(&params).ParseTokenIf(
            ExactType(RPAREN), &rparen);

    if (!afterParams) {
      ErrorList errors;
      FirstOf(&errors, &params, &rparen);
      return afterCommon.Fail(move(errors), out);
    }

    sptr<const Stmt> bodyPtr(nullptr);
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
      bodyPtr = body.Get();
    }

    sptr<const Type> typeptr = nullptr;
    if (!isConstructor) {
      typeptr = type.Get();
    }
    return afterBody.Success(
        new MethodDecl(*mods.Get(), typeptr, TokenString(file_, *ident.Get()),
            *ident.Get(), params.Get(), bodyPtr),
        out);
  }

  // Parse field.
  if (afterCommon.IsNext(SEMI)) {
    return afterCommon.Advance().Success(
        new FieldDecl(*mods.Get(), type.Get(), TokenString(file_, *ident.Get()),
            *ident.Get(), nullptr),
        out);
  }

  Result<Token> eq;
  Result<Expr> val;
  Result<Token> semi;
  Parser afterVal = afterCommon.ParseTokenIf(ExactType(ASSG), &eq)
                        .ParseExpression(&val)
                        .ParseTokenIf(ExactType(SEMI), &semi);

  RETURN_IF_GOOD(afterVal, new FieldDecl(*mods.Get(), type.Get(),
        TokenString(file_, *ident.Get()), *ident.Get(), val.Get()),
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

  SharedPtrVector<const Param> params;
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

    Parser afterIdent = afterType.ParseTokenIf(ExactType(IDENTIFIER), &ident);
    if (!afterIdent) {
      // Committed to getting identifier.
      return afterType.Fail(MakeParamRequiresNameError(cur.GetNext()), out);
    }
    cur = afterIdent;
    params.Append(make_shared<Param>(type.Get(), TokenString(file_, *ident.Get()), *ident.Get()));

    if (cur.IsNext(COMMA)) {
      cur = cur.Advance();
    } else {
      break;
    }
  }
  return cur.Success(new ParamList(params), out);
}

Parser Parser::ParseTypeDecl(Result<TypeDecl>* out) const {
  // TypeDeclaration:
  //   ClassDeclaration
  //   InterfaceDeclaration
  //   ";"
  // ClassDeclaration:
  //   ModifierList "class" Identifier ["extends" QualifiedName] ["implements" Interfaces] ClassBody
  // InterfaceDeclaration:
  //   ModifierList "interface" Identifier ["extends" Interfaces] ClassBody
  // Interfaces:
  //   QualifiedName {"," QualifiedName}

  SHORT_CIRCUIT;

  Result<ModifierList> mods;
  Parser afterMods = (*this).ParseModifierList(&mods);
  if (!afterMods) {
    ErrorList errors;
    mods.ReleaseErrors(&errors);
    return Fail(move(errors), out);
  }

  if (afterMods.IsAtEnd()) {
    return afterMods.Fail(MakeUnexpectedEOFError(), out);
  }

  if (!afterMods.IsNext(K_CLASS) && !afterMods.IsNext(K_INTERFACE)) {
    return Fail(MakeUnexpectedTokenError(afterMods.GetNext()), out);
  }

  Token typeToken = afterMods.GetNext();
  Parser afterType = afterMods.Advance();
  TypeKind kind = (typeToken.type == K_CLASS) ? TypeKind::CLASS : TypeKind::INTERFACE;

  Result<Token> ident;
  Parser afterIdent = afterType.ParseTokenIf(ExactType(IDENTIFIER), &ident);

  if (!afterIdent) {
    ErrorList errors;
    ident.ReleaseErrors(&errors);
    return Fail(move(errors), out);
  }

  vector<QualifiedName> extends;
  vector<QualifiedName> implements;

  Parser afterExtends = afterIdent;
  if (afterIdent.IsNext(K_EXTENDS)) {
    Result<QualifiedName> firstExtend;
    afterExtends = afterIdent.Advance() // Advancing past 'extends'.
      .ParseQualifiedName(&firstExtend);

    if (!afterExtends) {
      ErrorList errors;
      firstExtend.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }

    extends.push_back(*firstExtend.Get());

    // Only interfaces may have 1+ things in their extends list.
    while (kind == TypeKind::INTERFACE && afterExtends.IsNext(COMMA)) {
      Result<QualifiedName> nextExtend;
      afterExtends = afterExtends.Advance() // Advancing past comma.
        .ParseQualifiedName(&nextExtend);

      if (!afterExtends) {
        ErrorList errors;
        nextExtend.ReleaseErrors(&errors);
        return Fail(move(errors), out);
      }

      extends.push_back(*nextExtend.Get());
    }
  }

  Parser afterImplements = afterExtends;
  if (kind == TypeKind::CLASS && afterExtends.IsNext(K_IMPLEMENTS)) {
    Result<QualifiedName> firstImplements;
    afterImplements = afterExtends.Advance() // Advancing past 'extends'.
      .ParseQualifiedName(&firstImplements);

    if (!afterImplements) {
      ErrorList errors;
      firstImplements.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }

    implements.push_back(*firstImplements.Get());
    while (afterImplements.IsNext(COMMA)) {
      Result<QualifiedName> nextImplement;
      afterImplements = afterImplements.Advance() // Advancing past comma.
        .ParseQualifiedName(&nextImplement);

      if (!afterImplements) {
        ErrorList errors;
        nextImplement.ReleaseErrors(&errors);
        return Fail(move(errors), out);
      }

      implements.push_back(*nextImplement.Get());
    }
  }

  Result<Token> lbrace;
  Parser afterBrace = afterImplements.ParseTokenIf(ExactType(LBRACE), &lbrace);
  if (!afterBrace) {
    ErrorList errors;
    lbrace.ReleaseErrors(&errors);
    return afterImplements.Fail(move(errors), out);
  }

  SharedPtrVector<const MemberDecl> members;

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

    members.Append(member.Get());
    afterBody = afterMember;
  }

  Parser afterRbrace = afterBody.Advance();
  return afterRbrace.Success(new TypeDecl(*mods.Get(), kind, TokenString(GetFile(), *ident.Get()),
        *ident.Get(), extends, implements, members), out);
}

Parser Parser::ParseImportDecl(Result<ImportDecl>* out) const {
  // ImportDeclaration:
  //   "import" QualifiedName [".*"] ";"

  SHORT_CIRCUIT;

  vector<Token> tokens;
  bool isWildCard = false;

  Result<Token> import;
  Result<Token> ident;
  Parser cur = (*this).ParseTokenIf(ExactType(K_IMPORT), &import).ParseTokenIf(
      ExactType(IDENTIFIER), &ident);

  if (!cur) {
    ErrorList errors;
    FirstOf(&errors, &import, &ident);
    return Fail(move(errors), out);
  }
  tokens.push_back(*ident.Get());

  while (cur.IsNext(DOT)) {
    Token dot = cur.GetNext();
    // Advancing past dot.
    Parser next = cur.Advance();
    if (next.IsNext(MUL)) {
      // Advancing past *.
      cur = next.Advance();
      isWildCard = true;
      break;
    }

    Result<Token> nextIdent;
    next = next.ParseTokenIf(ExactType(IDENTIFIER), &nextIdent);
    if (!next) {
      ErrorList errors;
      nextIdent.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }

    tokens.push_back(dot);
    tokens.push_back(*nextIdent.Get());
    cur = next;
  }

  Result<Token> semi;
  Parser afterSemi = cur.ParseTokenIf(ExactType(SEMI), &semi);

  if (!afterSemi) {
    ErrorList errors;
    semi.ReleaseErrors(&errors);
    return Fail(move(errors), out);
  }

  return afterSemi.Success(
      new ImportDecl(MakeQualifiedName(GetFile(), tokens), isWildCard), out);
}

Parser Parser::ParseCompUnit(internal::Result<CompUnit>* out) const {
  // CompilationUnit:
  //   [PackageDeclaration] {ImportDeclaration} {TypeDeclaration}
  // PackageDeclaration:
  //   "package" QualifiedName ";"
  // QualifiedName:
  //   Identifier {"." Identifier}
  // ImportDeclaration:
  //   "import" QualifiedName [".*"] ";"

  SHORT_CIRCUIT;

  vector<ImportDecl> imports;
  SharedPtrVector<const TypeDecl> types;

  if (IsAtEnd()) {
    return Success(new CompUnit(nullptr, imports, types), out);
  }

  sptr<const QualifiedName> packageName(nullptr);
  Parser afterPackage = *this;
  if (IsNext(K_PACKAGE)) {
    Result<Token> package;
    Result<QualifiedName> name;
    Result<Token> semi;

    afterPackage = (*this)
                       .ParseTokenIf(ExactType(K_PACKAGE), &package)
                       .ParseQualifiedName(&name)
                       .ParseTokenIf(ExactType(SEMI), &semi);

    if (!afterPackage) {
      ErrorList errors;
      FirstOf(&errors, &package, &name, &semi);
      return Fail(move(errors), out);
    }

    packageName = name.Get();
  }

  Parser afterImports = afterPackage.EatSemis();
  while (afterImports.IsNext(K_IMPORT)) {
    Result<ImportDecl> import;
    afterImports = afterImports.ParseImportDecl(&import).EatSemis();

    if (!afterImports) {
      ErrorList errors;
      import.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }

    imports.push_back(*import.Get());
  }

  Parser afterTypes = afterImports;
  while (!afterTypes.IsAtEnd()) {
    Result<TypeDecl> type;
    afterTypes = afterTypes.ParseTypeDecl(&type).EatSemis();

    if (!afterTypes) {
      ErrorList errors;
      type.ReleaseErrors(&errors);
      return Fail(move(errors), out);
    }

    types.Append(type.Get());
  }

  return afterTypes.Success(
      new CompUnit(packageName, imports, types), out);
}

sptr<const Program> Parse(const FileSet* fs,
                          const vector<vector<lexer::Token>>& tokens,
                          ErrorList* error_out) {
  assert((uint)fs->Size() == tokens.size());

  SharedPtrVector<const CompUnit> units;
  bool failed = false;

  for (int i = 0; i < fs->Size(); ++i) {
    const File* file = fs->Get(i);
    const vector<Token>& filetoks = tokens[i];
    Result<CompUnit> unit;

    Parser parser(fs, file, &filetoks, 0);
    parser.ParseCompUnit(&unit);

    if (unit) {
      units.Append(unit.Get());
    } else {
      failed = true;
    }

    // Move all errors and warnings to the output list.
    unit.ReleaseErrors(error_out);
  }

  return make_shared<Program>(units);
}

// TODO: After we have types, need to ensure byte literals are within 8-bit
// signed two's complement.
// TODO: in for-loop initializers, for-loop incrementors, and top-level
// statements, we must ensure that they are either assignment, method
// invocation, or class creation, not other types of expressions (like
// boolean ops).
// TODO: Handle parsing empty files.
// TODO: The weeder must ensure that non-abstract classes cannot have abstract
// methods.
// TODO: Weed out array indexing into 'this'; i.e. ("this[3]").
// TODO: Weed out parens around assignment in blocks, for initializer, for
// update.
// TODO: "Integer[] a;" gives strange error - should say requires
// initialization.
// TODO: Fix cast expression parsing. '(gee)-d' should be a subtraction, not a
// cast.

}  // namespace parser
