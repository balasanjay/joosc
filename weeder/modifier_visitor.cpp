#include "weeder/modifier_visitor.h"

#include "ast/ast.h"
#include "base/macros.h"
#include "lexer/lexer.h"

using std::function;
using std::initializer_list;

using ast::EmptyStmt;
using ast::FieldDecl;
using ast::MethodDecl;
using ast::ModifierList;
using ast::TypeKind;
using ast::VisitResult;
using base::Error;
using base::ErrorList;
using lexer::ABSTRACT;
using lexer::FINAL;
using lexer::Modifier;
using lexer::NATIVE;
using lexer::PROTECTED;
using lexer::PUBLIC;
using lexer::STATIC;
using lexer::Token;

namespace weeder {

namespace {

Error* MakeInterfaceFieldError(Token token) {
  return MakeSimplePosRangeError(token.pos, "InterfaceFieldError",
                                 "An interface cannot contain any fields.");
}

Error* MakeInterfaceConstructorError(Token token) {
  return MakeSimplePosRangeError(token.pos, "InterfaceConstructorError",
                                 "An interface cannot contain a constructor.");
}

Error* MakeInterfaceMethodImplError(Token token) {
  return MakeSimplePosRangeError(token.pos, "InterfaceMethodImplError",
                                 "An interface method cannot have a body.");
}

Error* MakeInterfaceMethodModifierError(Token token) {
  CHECK(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      token.pos, "InterfaceMethodModifierError",
      "An interface method cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeConflictingAccessModError(Token token) {
  return MakeSimplePosRangeError(
      token.pos, "ConflictingAccessModError",
      "A declaration cannot have conflicting access modifiers.");
}

Error* MakeClassMemberNoAccessModError(Token token) {
  return MakeSimplePosRangeError(
      token.pos, "ClassMemberNoAccessModError",
      "A class member must be either public or protected.");
}

Error* MakeInterfaceMethodNoAccessModError(Token token) {
  return MakeSimplePosRangeError(token.pos,
                                 "InterfaceMethodNoAccessModError",
                                 "An interface member must be public.");
}

Error* MakeInterfaceNoAccessModError(Token token) {
  return MakeSimplePosRangeError(token.pos, "InterfaceNoAccessModError",
                                 "An interface must be public.");
}

Error* MakeClassNoAccessModError(Token token) {
  return MakeSimplePosRangeError(token.pos, "ClassNoAccessModError",
                                 "A class must be public.");
}

Error* MakeClassFieldModifierError(Token token) {
  CHECK(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      token.pos, "ClassFieldModifierError",
      "A class field cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassMethodEmptyError(Token token) {
  return MakeSimplePosRangeError(
      token.pos, "ClassMethodEmptyError",
      "A method must be native or abstract to have an empty body.");
}

Error* MakeClassMethodNotEmptyError(Token token) {
  return MakeSimplePosRangeError(
      token.pos, "ClassMethodNotEmptyError",
      "A native or abstract method must not have a body.");
}

Error* MakeClassMethodAbstractModifierError(Token token) {
  CHECK(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      token.pos, "ClassMethodAbstractModifierError",
      "An abstract method cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassMethodStaticFinalError(Token token) {
  return MakeSimplePosRangeError(token.pos, "ClassMethodStaticFinalError",
                                 "A static method cannot be final.");
}

Error* MakeClassMethodNativeNotStaticError(Token token) {
  return MakeSimplePosRangeError(token.pos,
                                 "ClassMethodNativeNotStaticError",
                                 "A native method must be static.");
}

Error* MakeClassModifierError(Token token) {
  CHECK(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      token.pos, "ClassModifierError",
      "A class cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeAbstractFinalClassError(Token token) {
  return MakeSimplePosRangeError(token.pos, "AbstractFinalClass",
                                 "A class cannot be both abstract and final.");
}

Error* MakeInterfaceModifierError(Token token) {
  CHECK(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      token.pos, "InterfaceModifierError",
      "An interface cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassConstructorModifierError(Token token) {
  CHECK(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      token.pos, "ClassConstructorModifierError",
      "A class constructor cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassConstructorEmptyError(Token token) {
  return MakeSimplePosRangeError(token.pos, "ClassConstructorEmptyError",
                                 "A constructor cannot have an empty body.");
}

inline void VerifyNoneOf(const ModifierList& mods,
                         ErrorList* out,
                         function<Error*(Token)> error_maker,
                         const initializer_list<Modifier>& disallowed) {
  for (auto mod : disallowed) {
    if (mods.HasModifier(mod)) {
      out->Append(error_maker(mods.GetModifierToken(mod)));
    }
  }
}

inline void VerifyOneOf(const ModifierList& mods,
                        ErrorList* out, Token token,
                        function<Error*(Token)> error_maker,
                        const initializer_list<Modifier>& oneof) {
  for (auto mod : oneof) {
    if (mods.HasModifier(mod)) {
      return;
    }
  }
  out->Append(error_maker(token));
}

void VerifyNoConflictingAccessMods(const ModifierList& mods, ErrorList* out) {
  if (!mods.HasModifier(PUBLIC) || !mods.HasModifier(PROTECTED)) {
    return;
  }

  out->Append(MakeConflictingAccessModError(mods.GetModifierToken(PUBLIC)));
  out->Append(
      MakeConflictingAccessModError(mods.GetModifierToken(PROTECTED)));
}

}  // namespace

VISIT_DEFN(ClassModifierVisitor, FieldDecl, decl,) {
  // Cannot be both public and protected.
  VerifyNoConflictingAccessMods(decl.Mods(), errors_);

  // Must be at least one of public or protected.
  VerifyOneOf(decl.Mods(), errors_, decl.NameToken(),
              MakeClassMemberNoAccessModError, {PUBLIC, PROTECTED});

  // Can't be abstract, final, or native.
  VerifyNoneOf(decl.Mods(), errors_, MakeClassFieldModifierError,
               {ABSTRACT, FINAL, NATIVE});

  return VisitResult::SKIP;
}

VISIT_DEFN(ClassModifierVisitor, MethodDecl, decl,) {
  bool constructor = decl.TypePtr() == nullptr;

  // Cannot be both public and protected.
  VerifyNoConflictingAccessMods(decl.Mods(), errors_);

  // Must be at least one of public or protected.
  VerifyOneOf(decl.Mods(), errors_, decl.NameToken(),
              MakeClassMemberNoAccessModError, {PUBLIC, PROTECTED});

  const ModifierList& mods = decl.Mods();

  // A method has a body if and only if it is neither abstract nor native.
  if (!constructor) {
    if (IS_CONST_REF(EmptyStmt, decl.Body())) {
      // Has an empty body; this implies it must be either abstract or native.
      if (!mods.HasModifier(ABSTRACT) && !mods.HasModifier(NATIVE)) {
        errors_->Append(MakeClassMethodEmptyError(decl.NameToken()));
      }
    } else {
      // Has a non-empty body; this implies it must not be abstract or native.
      if (mods.HasModifier(ABSTRACT) || mods.HasModifier(NATIVE)) {
        errors_->Append(MakeClassMethodNotEmptyError(decl.NameToken()));
      }
    }
  }

  // A constructor cannot be abstract, static, final, or native.
  if (constructor) {
    VerifyNoneOf(decl.Mods(), errors_, MakeClassConstructorModifierError,
                 {ABSTRACT, STATIC, FINAL, NATIVE});
  }

  // A constructor must have a body; i.e. it can't be ";".
  if (constructor && IS_CONST_REF(EmptyStmt, decl.Body())) {
    errors_->Append(MakeClassConstructorEmptyError(decl.NameToken()));
  }

  // An abstract method cannot be static or final.
  if (!constructor && mods.HasModifier(ABSTRACT)) {
    VerifyNoneOf(mods, errors_, MakeClassMethodAbstractModifierError,
                 {STATIC, FINAL});
  }

  // A static method cannot be final.
  if (!constructor && mods.HasModifier(STATIC) && mods.HasModifier(FINAL)) {
    errors_->Append(
        MakeClassMethodStaticFinalError(mods.GetModifierToken(FINAL)));
  }

  // A native method must be static.
  if (!constructor && mods.HasModifier(NATIVE) && !mods.HasModifier(STATIC)) {
    errors_->Append(MakeClassMethodNativeNotStaticError(
        mods.GetModifierToken(NATIVE)));
  }

  return VisitResult::SKIP;
}

VISIT_DEFN(InterfaceModifierVisitor, FieldDecl, decl,) {
  // An interface cannot contain fields.
  errors_->Append(MakeInterfaceFieldError(decl.NameToken()));
  return VisitResult::SKIP;
}

VISIT_DEFN(InterfaceModifierVisitor, MethodDecl, decl,) {
  // An interface cannot contain constructors.
  if (decl.TypePtr() == nullptr) {
    errors_->Append(MakeInterfaceConstructorError(decl.NameToken()));
    return VisitResult::SKIP;
  }

  // An interface method cannot be static, final, native, or protected.
  VerifyNoneOf(decl.Mods(), errors_, MakeInterfaceMethodModifierError,
               {PROTECTED, STATIC, FINAL, NATIVE});

  // Must be public.
  VerifyOneOf(decl.Mods(), errors_, decl.NameToken(),
              MakeInterfaceMethodNoAccessModError, {PUBLIC});

  // An interface method cannot have a body.
  if (!IS_CONST_REF(EmptyStmt, decl.Body())) {
    errors_->Append(MakeInterfaceMethodImplError(decl.NameToken()));
  }

  return VisitResult::SKIP;
}

REWRITE_DEFN(ModifierVisitor, TypeDecl, TypeDecl, decl, declptr) {
  if (decl.Kind() == TypeKind::CLASS) {
    // A class cannot be protected, static, or native.
    VerifyNoneOf(decl.Mods(), errors_, MakeClassModifierError,
                 {PROTECTED, STATIC, NATIVE});

    // Must be public.
    VerifyOneOf(decl.Mods(), errors_, decl.NameToken(),
                MakeClassNoAccessModError, {PUBLIC});

    // A class cannot be both abstract and final.
    if (decl.Mods().HasModifier(ABSTRACT) && decl.Mods().HasModifier(FINAL)) {
      errors_->Append(MakeAbstractFinalClassError(decl.NameToken()));
    }

    ClassModifierVisitor visitor(errors_);
    return visitor.Rewrite(declptr);
  }

  CHECK(decl.Kind() == TypeKind::INTERFACE);
  // An interface cannot be protected, static, final, or native.
  VerifyNoneOf(decl.Mods(), errors_, MakeInterfaceModifierError,
               {PROTECTED, STATIC, FINAL, NATIVE});

  // An interface must be public.
  VerifyOneOf(decl.Mods(), errors_, decl.NameToken(),
              MakeInterfaceNoAccessModError, {PUBLIC});

  InterfaceModifierVisitor visitor(errors_);
  return visitor.Rewrite(declptr);
}

}  // namespace weeder
