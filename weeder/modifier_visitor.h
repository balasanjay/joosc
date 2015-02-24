#ifndef WEEDER_MODIFIER_VISITOR_H
#define WEEDER_MODIFIER_VISITOR_H

#include "ast/visitor.h"
#include "base/errorlist.h"
#include "base/fileset.h"

namespace weeder {

// ModifierVisitor checks for misuse of modifiers. Depending on whether a
// CompilationUnit contains a ClassDecl or an InterfaceDecl, it will select one
// of ClassModifierVisitor or InterfaceModifier visitor, and traverse the
// declaration using that visitor.
class ModifierVisitor : public ast::Visitor {
 public:
  ModifierVisitor(const base::FileSet* fs, base::ErrorList* errors)
      : fs_(fs), errors_(errors) {}

  REWRITE_DECL(TypeDecl, TypeDecl, decl, declptr);

 private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

// ClassModifierVisitor checks the following rules:
//   1) A method has a body if and only if it is neither abstract nor native.
//   2) An abstract method cannot be static or final.
//   3) A static method cannot be final.
//   4) A native method must be static.
//   5) No field can be final, abstract, or native.
//   6) A class cannot be protected, static, or native.
//   7) A class cannot be both abstract and final.
//   8) A constructor cannot be abstract, static, final, or native.
//   9) A constructor must have a body; i.e. it can't be ";".
//   10) A class must be public.
//   11) A member must be public or protected.
class ClassModifierVisitor : public ast::Visitor {
 public:
  ClassModifierVisitor(const base::FileSet* fs, base::ErrorList* errors)
      : fs_(fs), errors_(errors) {}

  VISIT_DECL(FieldDecl, decl);
  VISIT_DECL(MethodDecl, decl);

 private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

// InterfaceModifierVisitor checks that all modifiers are valid.
//   1) An interface cannot contain fields or constructors.
//   2) An interface method cannot be static, final, native, or protected.
//   3) An interface method cannot have a body.
//   4) An interface cannot be protected, static, final, or native.
//   5) An interface must be public.
//   6) An interface method must be public.
class InterfaceModifierVisitor : public ast::Visitor {
 public:
  InterfaceModifierVisitor(const base::FileSet* fs, base::ErrorList* errors)
      : fs_(fs), errors_(errors) {}

  VISIT_DECL(FieldDecl, decl);
  VISIT_DECL(MethodDecl, decl);

 private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

}  // namespace weeder

#endif
