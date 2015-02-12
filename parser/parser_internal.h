#ifndef PARSER_PARSER_INTERNAL_H
#define PARSER_PARSER_INTERNAL_H

#include "base/errorlist.h"
#include "base/file.h"
#include "lexer/lexer.h"
#include "ast/ast.h"
#include "third_party/gtest/gtest.h"

namespace parser {
namespace internal {

template <typename T>
class Result final {
 public:
  Result() = default;
  Result(Result&& other) = default;
  Result& operator=(Result&& other) = default;

  explicit operator bool() const { return IsSuccess(); }

  bool IsSuccess() const { return !errors_.IsFatal(); }

  sptr<T> Get2() const {
    if (!IsSuccess()) {
      throw "Get2() from non-successful result.";
    }
    return data_;
  }

  void ReleaseErrors(base::ErrorList* out) {
    vector<base::Error*> errors;
    errors_.Release(&errors);
    for (auto err : errors) {
      out->Append(err);
    }
  }

  const base::ErrorList& Errors() const { return errors_; }

 private:
  DISALLOW_COPY_AND_ASSIGN(Result);

  Result(sptr<T> data) : success_(true), data_(data) {}
  Result(base::Error* err) { errors_.Append(err); }
  Result(base::ErrorList&& errors)
      : success_(false), errors_(std::forward<base::ErrorList>(errors)) {}

  template <typename U>
  friend Result<U> MakeSuccess(U* t);

  template <typename U>
  friend Result<U> MakeSuccess(sptr<U> t);

  template <typename U>
  friend Result<U> Failure(base::Error* e);

  template <typename U>
  friend Result<U> Failure(base::ErrorList&&);

  template <typename T1, typename T2>
  friend Result<T2> ConvertError(Result<T1>&&);

  bool success_ = false;
  sptr<T> data_;
  base::ErrorList errors_;
};

template <typename T>
Result<T> MakeSuccess(T* t) {
  return Result<T>(sptr<T>(t));
}
template <typename T>
Result<T> MakeSuccess(sptr<T> t) {
  return Result<T>(t);
}
template <typename T>
Result<T> Failure(base::Error* e) {
  return Result<T>(e);
}
template <typename T>
Result<T> Failure(base::ErrorList&& e) {
  return Result<T>(std::forward<base::ErrorList>(e));
}

template <typename T>
void FirstOf(base::ErrorList* out, T* result) {
  result->ReleaseErrors(out);
}

template <typename T, typename... Rest>
void FirstOf(base::ErrorList* out, T* first, Rest... rest) {
  if (first->Errors().Size() == 0) {
    return FirstOf(out, rest...);
  }

  first->ReleaseErrors(out);
}

template <typename T, typename U>
Result<U> ConvertError(Result<T>&& r) {
  return Result<U>(std::move(r.errors_));
}

}  // namespace internal

struct Parser {
  Parser(const base::FileSet* fs, const base::File* file,
         const vector<lexer::Token>* tokens, int index = 0, bool failed = false)
      : fs_(fs), file_(file), tokens_(tokens), index_(index), failed_(failed) {}

  explicit operator bool() const { return !failed_; }

  Parser ParseTokenIf(std::function<bool(lexer::Token)> pred,
                      internal::Result<lexer::Token>* out) const;

  // Type-related parsers.
  Parser ParseQualifiedName(internal::Result<ast::QualifiedName>* out) const;
  Parser ParsePrimitiveType(internal::Result<ast::Type>* out) const;
  Parser ParseSingleType(internal::Result<ast::Type>* out) const;
  Parser ParseType(internal::Result<ast::Type>* out) const;

  // Expression parsers.
  Parser ParseExpression(internal::Result<ast::Expr>*) const;
  Parser ParseParenExpression(internal::Result<ast::Expr>*) const;
  Parser ParseUnaryExpression(internal::Result<ast::Expr>*) const;
  Parser ParseCastExpression(internal::Result<ast::Expr>* out) const;
  Parser ParsePrimary(internal::Result<ast::Expr>* out) const;
  Parser ParseNewExpression(internal::Result<ast::Expr>* out) const;
  Parser ParsePrimaryBase(internal::Result<ast::Expr>* out) const;
  Parser ParsePrimaryEnd(sptr<ast::Expr> base, internal::Result<ast::Expr>* out) const;
  Parser ParsePrimaryEndNoArrayAccess(sptr<ast::Expr> base,
                                      internal::Result<ast::Expr>* out) const;

  // Other parsers.
  Parser ParseArgumentList(internal::Result<base::SharedPtrVector<ast::Expr>>*) const;

  // Statement parsers.
  Parser ParseStmt(internal::Result<ast::Stmt>* out) const;
  Parser ParseVarDecl(internal::Result<ast::Stmt>* out) const;
  Parser ParseReturnStmt(internal::Result<ast::Stmt>* out) const;
  Parser ParseBlock(internal::Result<ast::Stmt>* out) const;
  Parser ParseIfStmt(internal::Result<ast::Stmt>* out) const;
  Parser ParseForInit(internal::Result<ast::Stmt>* out) const;
  Parser ParseForStmt(internal::Result<ast::Stmt>* out) const;
  Parser ParseWhileStmt(internal::Result<ast::Stmt>* out) const;

  // Class parsers.
  Parser ParseModifierList(internal::Result<ast::ModifierList>* out) const;
  Parser ParseMemberDecl(internal::Result<ast::MemberDecl>* out) const;
  Parser ParseParamList(internal::Result<ast::ParamList>* out) const;
  Parser ParseTypeDecl(internal::Result<ast::TypeDecl>* out) const;

  // Compilation unit parsers.
  Parser ParseCompUnit(internal::Result<ast::CompUnit>* out) const;
  Parser ParseImportDecl(internal::Result<ast::ImportDecl>* out) const;

  // Helper methods.
  Parser EatSemis() const;

  bool IsAtEnd() const { return failed_ || (uint)index_ >= tokens_->size(); }

  bool Failed() const { return failed_; }

  lexer::Token GetNext() const {
    assert(!IsAtEnd());
    return tokens_->at(index_);
  }

 private:
  base::Error* MakeUnexpectedTokenError(lexer::Token token) const;
  base::Error* MakeDuplicateModifierError(lexer::Token token) const;
  base::Error* MakeParamRequiresNameError(lexer::Token token) const;
  base::Error* MakeUnexpectedEOFError() const;

  bool IsNext(lexer::TokenType type) const {
    return !IsAtEnd() && GetNext().type == type;
  }

  bool IsNext(std::function<bool(lexer::Token)> pred) const {
    return !IsAtEnd() && pred(GetNext());
  }

  Parser Advance(int i = 1) const {
    return Parser(fs_, file_, tokens_, index_ + i, failed_);
  }

  template <typename T>
  Parser Fail(base::Error* error, internal::Result<T>* out) const {
    *out = internal::Failure<T>(error);
    return Fail();
  }

  template <typename T>
  Parser Fail(base::ErrorList&& errors, internal::Result<T>* out) const {
    *out = internal::Failure<T>(std::forward<base::ErrorList>(errors));
    return Fail();
  }

  Parser Fail() const {
    return Parser(fs_, file_, tokens_, index_, /* failed */ true);
  }

  template <typename T, typename U>
  Parser Success(T* t, internal::Result<U>* out) const {
    *out = internal::MakeSuccess<U>(t);
    return *this;
  }

  template <typename T, typename U>
  Parser Success(T t, internal::Result<U>* out) const {
    *out = internal::MakeSuccess<U>(t);
    return *this;
  }

  const base::FileSet* Fs() const { return fs_; }
  const base::File* GetFile() const { return file_; }

  const base::FileSet* fs_ = nullptr;
  const base::File* file_ = nullptr;
  const vector<lexer::Token>* tokens_ = nullptr;
  int index_ = -1;
  bool failed_ = false;
};

}  // namespace parser

#endif
