#ifndef WEEDER_MODIFIER_VISITOR_H
#define WEEDER_MODIFIER_VISITOR_H

#include "base/fileset.h"
#include "base/errorlist.h"
#include "parser/recursive_visitor.h"

namespace weeder {

// ModifierVisitor checks for misuse of modifiers. Depending on whether a
// CompilationUnit contains a ClassDecl or an InterfaceDecl, it will select one
// of ClassModifierVisitor or InterfaceModifier visitor, and traverse the
// declaration using that visitor.
class ModifierVisitor : public parser::RecursiveVisitor {
public:
  ModifierVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(ClassDecl, decl);
  REC_VISIT_DECL(InterfaceDecl, decl);

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
class ClassModifierVisitor : public parser::RecursiveVisitor {
public:
  ClassModifierVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(FieldDecl, decl);
  REC_VISIT_DECL(MethodDecl, decl);

private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

// InterfaceModifierVisitor checks that all modifiers are valid.
//   1) An interface cannot contain fields or (TODO) constructors.
//   2) An interface method cannot be static, final, native, or protected.
//   3) An interface method cannot have a body.
//   4) An interface cannot be protected, static, final, or native.
class InterfaceModifierVisitor : public parser::RecursiveVisitor {
public:
  InterfaceModifierVisitor(const base::FileSet* fs, base::ErrorList* errors) : fs_(fs), errors_(errors) {}

  REC_VISIT_DECL(FieldDecl, decl);
  REC_VISIT_DECL(MethodDecl, decl);

private:
  const base::FileSet* fs_;
  base::ErrorList* errors_;
};

} // namespace weeder

#endif
