#include "base/macros.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "weeder/modifier_visitor.h"

using base::Error;
using base::ErrorList;
using base::FileSet;
using lexer::ABSTRACT;
using lexer::FINAL;
using lexer::Modifier;
using lexer::NATIVE;
using lexer::PROTECTED;
using lexer::PUBLIC;
using lexer::STATIC;
using lexer::Token;
using parser::EmptyStmt;
using parser::FieldDecl;
using parser::MethodDecl;
using parser::ModifierList;

namespace weeder {

namespace {

Error* MakeInterfaceFieldError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "InterfaceFieldError",
      "An interface cannot contain any fields.");
}

Error* MakeInterfaceMethodImplError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "InterfaceMethodImplError",
      "An interface method cannot have a body.");
}

Error* MakeInterfaceMethodModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos,
      "InterfaceMethodModifierError",
      "An interface method cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeConflictingAccessModError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "ConflictingAccessModError",
      "A declaration cannot have conflicting access modifiers.");
}

Error* MakeClassFieldModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos,
      "ClassFieldModifierError",
      "A class field cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassMethodEmptyError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "ClassMethodEmptyError",
      "A method must be native or abstract to have an empty body.");
}

Error* MakeClassMethodNotEmptyError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "ClassMethodNotEmptyError",
      "A native or static method must not have a body.");
}

Error* MakeClassMethodAbstractModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos,
      "ClassMethodAbstractModifierError",
      "An abstract method cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassMethodStaticFinalError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "ClassMethodStaticFinalError",
      "A static method cannot be final.");
}

Error* MakeClassMethodNativeNotStaticError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos,
      "ClassMethodNativeNotStaticError",
      "A native method must be static.");
}

inline void VerifyNoneOf(
    const FileSet*, const ModifierList&, ErrorList*,
    std::function<Error*(const FileSet*, Token)>) {
  // Base-case for 0 modifiers, meant to be empty.
}

template <typename... T>
inline void VerifyNoneOf(
    const FileSet* fs, const ModifierList& mods, ErrorList* out,
    std::function<Error*(const FileSet*, Token)> error_maker,
    Modifier disallowed, T... othermodifiers) {
  if (mods.HasModifier(disallowed)) {
    out->Append(error_maker(fs, mods.GetModifierToken(disallowed)));
  }

  VerifyNoneOf(fs, mods, out, error_maker, othermodifiers...);
}

void VerifyNoConflictingAccessMods(const FileSet* fs, const ModifierList& mods, ErrorList* out) {
  if (!mods.HasModifier(PUBLIC) || !mods.HasModifier(PROTECTED)) {
    return;
  }

  out->Append(MakeConflictingAccessModError(fs, mods.GetModifierToken(PUBLIC)));
  out->Append(MakeConflictingAccessModError(fs, mods.GetModifierToken(PROTECTED)));
}

} // namespace

REC_VISIT_DEFN(ClassModifierVisitor, FieldDecl, decl) {
  // Cannot be both public and protected.
  VerifyNoConflictingAccessMods(fs_, decl->Mods(), errors_);
  // Can't be abstract, final, or native.
  VerifyNoneOf(
      fs_, decl->Mods(), errors_, MakeClassFieldModifierError,
      ABSTRACT, FINAL, NATIVE);
  return false;
}

REC_VISIT_DEFN(ClassModifierVisitor, MethodDecl, decl) {
  // Cannot be both public and protected.
  VerifyNoConflictingAccessMods(fs_, decl->Mods(), errors_);

  const ModifierList& mods = decl->Mods();

  // A method has a body if and only if it is neither abstract nor native.
  {
    if (IS_CONST_PTR(EmptyStmt, decl->Body())) {
      // Has an empty body; this implies it must be either abstract or native.
      if (!mods.HasModifier(ABSTRACT) && !mods.HasModifier(NATIVE)) {
        errors_->Append(MakeClassMethodEmptyError(fs_, decl->Ident()));
      }
    } else {
      // Has a non-empty body; this implies it must not be abstract or native.
      if (mods.HasModifier(ABSTRACT) || mods.HasModifier(NATIVE)) {
        errors_->Append(MakeClassMethodNotEmptyError(fs_, decl->Ident()));
      }
    }
  }

  // An abstract method cannot be static or final.
  if (mods.HasModifier(ABSTRACT)) {
    VerifyNoneOf(
        fs_, mods, errors_, MakeClassMethodAbstractModifierError,
        STATIC, FINAL);
  }

  // A static method cannot be final.
  if (mods.HasModifier(STATIC) && mods.HasModifier(FINAL)) {
    errors_->Append(MakeClassMethodStaticFinalError(fs_, mods.GetModifierToken(FINAL)));
  }

  // A native method must be static.
  if (mods.HasModifier(NATIVE) && !mods.HasModifier(STATIC)) {
    errors_->Append(MakeClassMethodNativeNotStaticError(fs_, mods.GetModifierToken(NATIVE)));
  }

  return false;
}

REC_VISIT_DEFN(InterfaceModifierVisitor, FieldDecl, decl) {
  // An interface cannot contain fields.
  errors_->Append(MakeInterfaceFieldError(fs_, decl->Ident()));
  return false;
}

REC_VISIT_DEFN(InterfaceModifierVisitor, MethodDecl, decl) {
  // An interface method cannot be static, final, native, or protected.
  VerifyNoneOf(
      fs_, decl->Mods(), errors_, MakeInterfaceMethodModifierError,
      PROTECTED, STATIC, FINAL, NATIVE);

  // An interface method cannot have a body.
  if (!IS_CONST_PTR(EmptyStmt, decl->Body())) {
    errors_->Append(MakeInterfaceMethodImplError(fs_, decl->Ident()));
  }

  return false;
}

} // namespace weeder
