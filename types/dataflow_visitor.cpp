#include "types/dataflow_visitor.h"

#include "base/error.h"
#include "ast/extent.h"

using ast::Expr;
using ast::FieldDerefExpr;
using ast::FieldId;
using ast::EmptyStmt;
using ast::Stmt;
using ast::ThisExpr;
using ast::TypeId;
using ast::VisitResult;
using base::DiagnosticClass;
using base::Error;
using base::ErrorList;
using base::FileSet;
using base::MakeError;
using base::OutputOptions;
using base::PosRange;

namespace types {

namespace {

bool IsConstantBool(sptr<const Expr> expr, bool want) {
  // TODO: Constant folding.
  const ast::BoolLitExpr* bool_expr = dynamic_cast<const ast::BoolLitExpr*>(expr.get());
  if (bool_expr == nullptr) {
    return false;
  }
  bool is_true = bool_expr->GetToken().type == lexer::K_TRUE;
  return is_true == want;
}

Error* MakeFieldOrderError(PosRange usage, PosRange decl) {
  return MakeError([=](std::ostream* out, const OutputOptions& opt, const FileSet* fs) {
    if (opt.simple) {
      *out << "FieldOrderError(" << usage << "," << decl << ")";
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, usage, DiagnosticClass::ERROR, "Field used before declaration completed.");
    PrintRangePtr(out, opt, fs, usage);
    *out << '\n';
    PrintDiagnosticHeader(out, opt, fs, decl, DiagnosticClass::INFO, "Declared here.");
    PrintRangePtr(out, opt, fs, decl);
  });
}


class FieldOrderVisitor final : public ast::Visitor {
 public:
  FieldOrderVisitor(const TypeInfoMap& typeinfo, ErrorList* errors, TypeId curtype, FieldId curfield) : typeinfo_(typeinfo), errors_(errors), curtype_(curtype), curfield_(curfield) {}

  VISIT_DECL(FieldDerefExpr, expr,) {
    // If the field is not owned by the current type decl, allow any order.
    if (expr.Base().GetTypeId() != curtype_) {
      return VisitResult::RECURSE;
    }

    FieldId fid = expr.GetFieldId();
    const FieldInfo& finfo = typeinfo_.LookupTypeInfo(curtype_).fields.LookupField(fid);

    // If this field is inherited, allow any order.
    if (finfo.class_type != curtype_) {
      return VisitResult::RECURSE;
    }

    // If this field is static, allow any order.
    if (finfo.mods.HasModifier(lexer::Modifier::STATIC)) {
      return VisitResult::RECURSE;
    }

    // If we didn't access this field with a simple name (implicit this), allow any order.
    {
      const ThisExpr* base = dynamic_cast<const ThisExpr*>(expr.BasePtr().get());
      if (base == nullptr || !base->IsImplicit()) {
        return VisitResult::RECURSE;
      }
    }

    // Check if we're accessing the current field in its own expr or a field defined later in the file.
    if (fid >= curfield_) {
      errors_->Append(MakeFieldOrderError(expr.GetToken().pos, finfo.pos));
    }
    return VisitResult::RECURSE;
  }

  // Immediate LHS of assignment is exempt from field order checks.
  VISIT_DECL(BinExpr, expr,) {
    if (expr.Op().type != lexer::ASSG) {
      return VisitResult::RECURSE;
    }

    const FieldDerefExpr* deref = dynamic_cast<const FieldDerefExpr*>(expr.LhsPtr().get());
    if (deref == nullptr) {
      return VisitResult::RECURSE;
    }

    Visit(deref->BasePtr());
    Visit(expr.RhsPtr());
    return VisitResult::SKIP;
  }

 private:
  const TypeInfoMap& typeinfo_;
  ErrorList* errors_;
  TypeId curtype_;
  FieldId curfield_;
};

class ReachabilityVisitor final : public ast::Visitor {
 public:
  ReachabilityVisitor(ErrorList* errors) : errors_(errors) {}

  VISIT_DECL(BlockStmt, stmt,) {
    for (const auto& substmt : stmt.Stmts().Vec()) {
      CheckReachable(ExtentOf(substmt));
      Visit(substmt);
    }
    may_emit_ = true;

    return VisitResult::SKIP;
  }

  VISIT_DECL(ReturnStmt,,) {
    reachable_ = false;
    return VisitResult::SKIP;
  }

  VISIT_DECL(IfStmt, stmt,) {
    ReachabilityVisitor true_visitor = Nested();
    true_visitor.Visit(stmt.TrueBodyPtr());

    ReachabilityVisitor false_visitor = Nested();
    false_visitor.Visit(stmt.FalseBodyPtr());

    // Code after this if is unreachable if both branches return.
    reachable_ = true_visitor.reachable_ || false_visitor.reachable_;

    return VisitResult::SKIP;
  }

  VISIT_DECL(ForStmt, stmt,) {
    return VisitLoop(stmt.CondPtr(), stmt.BodyPtr());
  }

  VISIT_DECL(WhileStmt, stmt,) {
    return VisitLoop(stmt.CondPtr(), stmt.BodyPtr());
  }

  VISIT_DECL(MethodDecl, member,) {
    sptr<const Stmt> body = member.BodyPtr();
    if (dynamic_cast<const EmptyStmt*>(body.get()) != nullptr) {
      return VisitResult::SKIP;
    }

    Visit(body);

    bool is_void = member.TypePtr() == nullptr || member.TypePtr()->GetTypeId() == TypeId::kVoid;
    if (reachable_ && !is_void) {
      errors_->Append(MakeSimplePosRangeError(member.NameToken().pos, "MethodNeedsReturnError", "Can reach end of method without returning a value."));
    }

    return VisitResult::SKIP;
  }

 private:
  ReachabilityVisitor Nested() {
    return Nested(reachable_, may_emit_);
  }

  ReachabilityVisitor Nested(bool reachable, bool may_emit) {
    ReachabilityVisitor v(errors_);
    v.reachable_ = reachable;
    v.may_emit_ = may_emit;
    return v;
  }

  void CheckReachable(PosRange pos) {
    if (!reachable_ && may_emit_) {
      may_emit_ = false;
      errors_->Append(MakeSimplePosRangeError(pos, "UnreachableCodeError", "Unreachable code."));
    }
  }

  VisitResult VisitLoop(sptr<const Expr> cond, sptr<const Stmt> body) {
    bool is_const = true;
    bool const_val = true;

    if (cond != nullptr) {
      if (IsConstantBool(cond, true)) {
        const_val = true;
      } else if (IsConstantBool(cond, false)) {
        const_val = false;
      } else {
        is_const = false;
      }
    }

    // Can't enter loop.
    if (is_const && !const_val) {
      ReachabilityVisitor nested = Nested(false, may_emit_);
      nested.CheckReachable(ExtentOf(body));
      reachable_ = true;
      return VisitResult::SKIP;
    }

    // Infinite loop but might return inside.
    if (is_const && const_val) {
      ReachabilityVisitor nested = Nested();
      nested.Visit(body);
      reachable_ = false;
      return VisitResult::SKIP;
    }

    // Might run, might not; might exit, might not.
    CHECK(!is_const);
    ReachabilityVisitor nested = Nested();
    nested.Visit(body);

    // We might exit from the loop or skip it if it has a return, so propagate same reachability.

    return VisitResult::SKIP;
  }

  ErrorList* errors_;
  bool reachable_ = true;
  bool may_emit_ = true;
};


} // namespace


VISIT_DEFN(DataflowVisitor, TypeDecl, decl, declptr) {
  // If we've already set the current type, just recurse.
  if (curtype_ != TypeId::kUnassigned) {
    return VisitResult::RECURSE;
  }

  CHECK(decl.GetTypeId() != TypeId::kUnassigned);
  DataflowVisitor nested(typeinfo_, errors_, decl.GetTypeId());
  nested.Visit(declptr);
  return VisitResult::SKIP;
}

VISIT_DEFN(DataflowVisitor, FieldDecl, decl, declptr) {
  FieldOrderVisitor nested(typeinfo_, errors_, curtype_, decl.GetFieldId());
  nested.Visit(declptr);
  return VisitResult::SKIP;
}

VISIT_DEFN(DataflowVisitor, MethodDecl, , declptr) {
  ReachabilityVisitor nested(errors_);
  nested.Visit(declptr);
  return VisitResult::SKIP;
}

} // namespace types

