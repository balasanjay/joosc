#ifndef TYPING_FUN_REWRITER_H
#define TYPING_FUN_REWRITER_H

#include "ast/ast.h"
#include "ast/rewriter.h"

namespace typing {

class FunRewriter : public ast::Rewriter {
  REWRITE_DECL(BoolLitExpr, Expr, expr);
  REWRITE_DECL(ClassDecl, TypeDecl, expr);
};

REWRITE_DEFN(FunRewriter, BoolLitExpr, Expr, expr) {
  lexer::Token tok = expr.GetToken();
  if (tok.TypeInfo().Type() == lexer::K_TRUE) {
    return new ast::BoolLitExpr(lexer::Token(lexer::K_FALSE, tok.pos));
  } else {
    return new ast::BoolLitExpr(lexer::Token(lexer::K_TRUE, tok.pos));
  }
}

REWRITE_DEFN(FunRewriter, ClassDecl, TypeDecl, type) {
  ast::ModifierList mods(type.Mods());
  base::UniquePtrVector<ast::ReferenceType> interfaces;
  for (int i = 0; i < type.Interfaces().Size(); ++i) {
    interfaces.Append(type.Interfaces().At(i)->Clone());
  }
  base::UniquePtrVector<ast::MemberDecl> members;
  for (int i = 0; i < type.Members().Size(); ++i) {
    members.Append(type.Members().At(i)->AcceptRewriter(this));
  }
  ast::ReferenceType* super = nullptr;
  if (type.Super() != nullptr) {
    super = static_cast<ast::ReferenceType*>(type.Super()->Clone());
  }
  return new ast::ClassDecl(std::move(mods), "Fail" + type.Name(), type.NameToken(), std::move(interfaces), std::move(members), super);
}

} // namespace typing

#endif
