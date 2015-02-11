#ifndef TYPING_FUN_REWRITER_H
#define TYPING_FUN_REWRITER_H

#include "parser/ast.h"
#include "typing/rewriter.h"

namespace parser {

class FunRewriter : public Rewriter {
  REWRITE_DECL(BoolLitExpr, Expr, expr);
  REWRITE_DECL(ClassDecl, TypeDecl, expr);
};

REWRITE_DEFN(FunRewriter, BoolLitExpr, Expr, expr) {
  lexer::Token tok = expr.GetToken();
  if (tok.TypeInfo().Type() == lexer::K_TRUE) {
    return new BoolLitExpr(lexer::Token(lexer::K_FALSE, tok.pos));
  } else {
    return new BoolLitExpr(lexer::Token(lexer::K_TRUE, tok.pos));
  }
}

REWRITE_DEFN(FunRewriter, ClassDecl, TypeDecl, type) {
  ModifierList mods(type.Mods());
  base::UniquePtrVector<ReferenceType> interfaces;
  for (int i = 0; i < type.Interfaces().Size(); ++i) {
    interfaces.Append(static_cast<ReferenceType*>(type.Interfaces().At(i)->clone()));
  }
  base::UniquePtrVector<MemberDecl> members;
  for (int i = 0; i < type.Members().Size(); ++i) {
    members.Append(type.Members().At(i)->RewriteAccept(this));
  }
  ReferenceType* super = nullptr;
  if (type.Super() != nullptr) {
    super = static_cast<ReferenceType*>(type.Super()->clone());
  }
  return new ClassDecl(std::move(mods), "Fail" + type.Name(), type.NameToken(), std::move(interfaces), std::move(members), super);
}

}

#endif
