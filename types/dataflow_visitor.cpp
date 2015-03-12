#include "types/dataflow_visitor.h"

#include "base/error.h"

using ast::FieldDerefExpr;
using ast::FieldId;
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

VISIT_DEFN(DataflowVisitor, MethodDecl, ,) {
  // TODO: Check statment reachability with another visitor.
  return VisitResult::SKIP;
}

} // namespace types

