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
using base::FileSet;
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

Error* MakeInterfaceFieldError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "InterfaceFieldError",
                                 "An interface cannot contain any fields.");
}

Error* MakeInterfaceConstructorError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "InterfaceConstructorError",
                                 "An interface cannot contain a constructor.");
}

Error* MakeInterfaceMethodImplError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "InterfaceMethodImplError",
                                 "An interface method cannot have a body.");
}

Error* MakeInterfaceMethodModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos, "InterfaceMethodModifierError",
      "An interface method cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeConflictingAccessModError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "ConflictingAccessModError",
      "A declaration cannot have conflicting access modifiers.");
}

Error* MakeClassMemberNoAccessModError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "ClassMemberNoAccessModError",
      "A class member must be either public or protected.");
}

Error* MakeInterfaceMethodNoAccessModError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos,
                                 "InterfaceMethodNoAccessModError",
                                 "An interface member must be public.");
}

Error* MakeInterfaceNoAccessModError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "InterfaceNoAccessModError",
                                 "An interface must be public.");
}

Error* MakeClassNoAccessModError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "ClassNoAccessModError",
                                 "A class must be public.");
}

Error* MakeClassFieldModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos, "ClassFieldModifierError",
      "A class field cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassMethodEmptyError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "ClassMethodEmptyError",
      "A method must be native or abstract to have an empty body.");
}

Error* MakeClassMethodNotEmptyError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(
      fs, token.pos, "ClassMethodNotEmptyError",
      "A native or abstract method must not have a body.");
}

Error* MakeClassMethodAbstractModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos, "ClassMethodAbstractModifierError",
      "An abstract method cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassMethodStaticFinalError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "ClassMethodStaticFinalError",
                                 "A static method cannot be final.");
}

Error* MakeClassMethodNativeNotStaticError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos,
                                 "ClassMethodNativeNotStaticError",
                                 "A native method must be static.");
}

Error* MakeClassModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos, "ClassModifierError",
      "A class cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeAbstractFinalClassError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "AbstractFinalClass",
                                 "A class cannot be both abstract and final.");
}

Error* MakeInterfaceModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos, "InterfaceModifierError",
      "An interface cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassConstructorModifierError(const FileSet* fs, Token token) {
  assert(token.TypeInfo().IsModifier());
  return MakeSimplePosRangeError(
      fs, token.pos, "ClassConstructorModifierError",
      "A class constructor cannot be " + token.TypeInfo().Value() + ".");
}

Error* MakeClassConstructorEmptyError(const FileSet* fs, Token token) {
  return MakeSimplePosRangeError(fs, token.pos, "ClassConstructorEmptyError",
                                 "A constructor cannot have an empty body.");
}

inline void VerifyNoneOf(const FileSet* fs, const ModifierList& mods,
                         ErrorList* out,
                         function<Error*(const FileSet*, Token)> error_maker,
                         const initializer_list<Modifier>& disallowed) {
  for (auto mod : disallowed) {
    if (mods.HasModifier(mod)) {
      out->Append(error_maker(fs, mods.GetModifierToken(mod)));
    }
  }
}

inline void VerifyOneOf(const FileSet* fs, const ModifierList& mods,
                        ErrorList* out, Token token,
                        function<Error*(const FileSet*, Token)> error_maker,
                        const initializer_list<Modifier>& oneof) {
  for (auto mod : oneof) {
    if (mods.HasModifier(mod)) {
      return;
    }
  }
  out->Append(error_maker(fs, token));
}

void VerifyNoConflictingAccessMods(const FileSet* fs, const ModifierList& mods,
                                   ErrorList* out) {
  if (!mods.HasModifier(PUBLIC) || !mods.HasModifier(PROTECTED)) {
    return;
  }

  out->Append(MakeConflictingAccessModError(fs, mods.GetModifierToken(PUBLIC)));
  out->Append(
      MakeConflictingAccessModError(fs, mods.GetModifierToken(PROTECTED)));
}

}  // namespace

VISIT_DEFN(ClassModifierVisitor, FieldDecl, decl,) {
  // Cannot be both public and protected.
  VerifyNoConflictingAccessMods(fs_, decl.Mods(), errors_);

  // Must be at least one of public or protected.
  VerifyOneOf(fs_, decl.Mods(), errors_, decl.NameToken(),
              MakeClassMemberNoAccessModError, {PUBLIC, PROTECTED});

  // Can't be abstract, final, or native.
  VerifyNoneOf(fs_, decl.Mods(), errors_, MakeClassFieldModifierError,
               {ABSTRACT, FINAL, NATIVE});

  return VisitResult::SKIP;
}

VISIT_DEFN(ClassModifierVisitor, MethodDecl, decl,) {
  bool constructor = decl.TypePtr() == nullptr;

  // Cannot be both public and protected.
  VerifyNoConflictingAccessMods(fs_, decl.Mods(), errors_);

  // Must be at least one of public or protected.
  VerifyOneOf(fs_, decl.Mods(), errors_, decl.NameToken(),
              MakeClassMemberNoAccessModError, {PUBLIC, PROTECTED});

  const ModifierList& mods = decl.Mods();

  // A method has a body if and only if it is neither abstract nor native.
  if (!constructor) {
    if (IS_CONST_REF(EmptyStmt, decl.Body())) {
      // Has an empty body; this implies it must be either abstract or native.
      if (!mods.HasModifier(ABSTRACT) && !mods.HasModifier(NATIVE)) {
        errors_->Append(MakeClassMethodEmptyError(fs_, decl.NameToken()));
      }
    } else {
      // Has a non-empty body; this implies it must not be abstract or native.
      if (mods.HasModifier(ABSTRACT) || mods.HasModifier(NATIVE)) {
        errors_->Append(MakeClassMethodNotEmptyError(fs_, decl.NameToken()));
      }
    }
  }

  // A constructor cannot be abstract, static, final, or native.
  if (constructor) {
    VerifyNoneOf(fs_, decl.Mods(), errors_, MakeClassConstructorModifierError,
                 {ABSTRACT, STATIC, FINAL, NATIVE});
  }

  // A constructor must have a body; i.e. it can't be ";".
  if (constructor && IS_CONST_REF(EmptyStmt, decl.Body())) {
    errors_->Append(MakeClassConstructorEmptyError(fs_, decl.NameToken()));
  }

  // An abstract method cannot be static or final.
  if (!constructor && mods.HasModifier(ABSTRACT)) {
    VerifyNoneOf(fs_, mods, errors_, MakeClassMethodAbstractModifierError,
                 {STATIC, FINAL});
  }

  // A static method cannot be final.
  if (!constructor && mods.HasModifier(STATIC) && mods.HasModifier(FINAL)) {
    errors_->Append(
        MakeClassMethodStaticFinalError(fs_, mods.GetModifierToken(FINAL)));
  }

  // A native method must be static.
  if (!constructor && mods.HasModifier(NATIVE) && !mods.HasModifier(STATIC)) {
    errors_->Append(MakeClassMethodNativeNotStaticError(
        fs_, mods.GetModifierToken(NATIVE)));
  }

  return VisitResult::SKIP;
}

VISIT_DEFN(InterfaceModifierVisitor, FieldDecl, decl,) {
  // An interface cannot contain fields.
  errors_->Append(MakeInterfaceFieldError(fs_, decl.NameToken()));
  return VisitResult::SKIP;
}

VISIT_DEFN(InterfaceModifierVisitor, MethodDecl, decl,) {
  // An interface cannot contain constructors.
  if (decl.TypePtr() == nullptr) {
    errors_->Append(MakeInterfaceConstructorError(fs_, decl.NameToken()));
    return VisitResult::SKIP;
  }

  // An interface method cannot be static, final, native, or protected.
  VerifyNoneOf(fs_, decl.Mods(), errors_, MakeInterfaceMethodModifierError,
               {PROTECTED, STATIC, FINAL, NATIVE});

  // Must be public.
  VerifyOneOf(fs_, decl.Mods(), errors_, decl.NameToken(),
              MakeInterfaceMethodNoAccessModError, {PUBLIC});

  // An interface method cannot have a body.
  if (!IS_CONST_REF(EmptyStmt, decl.Body())) {
    errors_->Append(MakeInterfaceMethodImplError(fs_, decl.NameToken()));
  }

  return VisitResult::SKIP;
}

REWRITE_DEFN(ModifierVisitor, TypeDecl, TypeDecl, decl, declptr) {
  if (decl.Kind() == TypeKind::CLASS) {
    // A class cannot be protected, static, or native.
    VerifyNoneOf(fs_, decl.Mods(), errors_, MakeClassModifierError,
                 {PROTECTED, STATIC, NATIVE});

    // Must be public.
    VerifyOneOf(fs_, decl.Mods(), errors_, decl.NameToken(),
                MakeClassNoAccessModError, {PUBLIC});

    // A class cannot be both abstract and final.
    if (decl.Mods().HasModifier(ABSTRACT) && decl.Mods().HasModifier(FINAL)) {
      errors_->Append(MakeAbstractFinalClassError(fs_, decl.NameToken()));
    }

    ClassModifierVisitor visitor(fs_, errors_);
    return visitor.Rewrite(declptr);
  }

  assert(decl.Kind() == TypeKind::INTERFACE);
  // An interface cannot be protected, static, final, or native.
  VerifyNoneOf(fs_, decl.Mods(), errors_, MakeInterfaceModifierError,
               {PROTECTED, STATIC, FINAL, NATIVE});

  // An interface must be public.
  VerifyOneOf(fs_, decl.Mods(), errors_, decl.NameToken(),
              MakeInterfaceNoAccessModError, {PUBLIC});

  InterfaceModifierVisitor visitor(fs_, errors_);
  return visitor.Rewrite(declptr);
}

}  // namespace weeder
