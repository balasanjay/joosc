#include "types/type_info_map.h"

#include "ast/ast.h"
#include "base/algorithm.h"
#include "lexer/lexer.h"
#include "types/types_internal.h"

using std::function;
using std::initializer_list;
using std::ostream;
using std::tie;

using ast::FieldId;
using ast::MethodId;
using ast::ModifierList;
using ast::TypeId;
using ast::TypeKind;
using ast::kArrayLengthFieldId;
using ast::kErrorFieldId;
using ast::kErrorMethodId;
using ast::kFirstFieldId;
using ast::kFirstMethodId;
using ast::kUnassignedMethodId;
using base::DiagnosticClass;
using base::Error;
using base::ErrorList;
using base::FindEqualRanges;
using base::MakeError;
using base::OutputOptions;
using base::PosRange;
using lexer::ABSTRACT;
using lexer::FINAL;
using lexer::K_ABSTRACT;
using lexer::K_FINAL;
using lexer::K_PROTECTED;
using lexer::K_PUBLIC;
using lexer::NATIVE;
using lexer::PROTECTED;
using lexer::PUBLIC;
using lexer::Token;

namespace types {

namespace {

bool IsAccessible(const TypeInfoMap& types, const ModifierList& mods, CallContext ctx, TypeId owner, TypeId caller, TypeId callee) {
  if (caller == owner) {
    return true;
  }
  if (!mods.HasModifier(lexer::PROTECTED)) {
    return true;
  }

  const TypeInfo& method_owner_tinfo = types.LookupTypeInfo(owner);
  const TypeInfo& caller_tinfo = types.LookupTypeInfo(caller);
  bool is_same_package = (method_owner_tinfo.package == caller_tinfo.package);

  // All protected members accessible from same package.
  if (is_same_package) {
    return true;
  }
  // Protected constructor never accessible outside of package.
  if (ctx == CallContext::CONSTRUCTOR) {
    return false;
  }
  // Accessible only if caller is subtype of owner.
  if (!types.IsAncestor(caller, owner)) {
    return false;
  }
  // It is sufficient that you are a descendent of the owner of a static member.
  if (ctx == CallContext::STATIC) {
    return true;
  }
  // Instance members are accessible only if callee is subtype of caller.
  return callee == caller || types.IsAncestor(callee, caller);
}

Error* MakeInterfaceExtendsClassError(PosRange pos, const string& parent_class) {
  string msg = "An interface may not extend '" + parent_class + "', a class.";
  return MakeSimplePosRangeError(pos, "InterfaceExtendsClassError", msg);
}

Error* MakeClassImplementsClassError(PosRange pos, const string& parent_class) {
  string msg = "A class may not implement '" + parent_class + "', a class.";
  return MakeSimplePosRangeError(pos, "ClassImplementsClassError", msg);
}

Error* MakeClassExtendsInterfaceError(PosRange pos, const string& parent_iface) {
  string msg = "A class may not extend '" + parent_iface + "', an interface.";
  return MakeSimplePosRangeError(pos, "ClassExtendInterfaceError", msg);
}

Error* MakeSimpleMethodTableError(PosRange m_pos, const string& m_string, PosRange p_pos, const string& p_string, const string& error_name) {
  return MakeError([=](ostream* out, const OutputOptions& opt, const base::FileSet* fs) {
    if (opt.simple) {
      *out << error_name << ": [" << m_pos << ',' << p_pos << ']';
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, m_pos, DiagnosticClass::ERROR, m_string);
    PrintRangePtr(out, opt, fs, m_pos);
    *out << '\n';
    PrintDiagnosticHeader(out, opt, fs, p_pos, DiagnosticClass::INFO, p_string);
    PrintRangePtr(out, opt, fs, p_pos);
  });
}

Error* MakeResolveMethodTableError(const TypeInfo& mtinfo, const MethodInfo& mminfo, const MethodInfo& pminfo, const string& m_string, const string& p_string, const string& error_name) {
  return MakeError([=](ostream* out, const OutputOptions& opt, const base::FileSet* fs) {
    bool is_self_method = (mtinfo.type == mminfo.class_type);
    PosRange m_pos = is_self_method ? mminfo.pos : mtinfo.pos;
    if (opt.simple) {
      *out << error_name << ": [" << m_pos << ',';
      if (is_self_method) {
        *out << pminfo.pos;
      } else {
        *out << mminfo.pos << ',' << pminfo.pos;
      }
      *out << ']';
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, m_pos, DiagnosticClass::ERROR, m_string);
    PrintRangePtr(out, opt, fs, m_pos);
    *out << '\n';
    if (is_self_method) {
      PrintDiagnosticHeader(out, opt, fs, pminfo.pos, DiagnosticClass::INFO, p_string);
      PrintRangePtr(out, opt, fs, pminfo.pos);
    } else {
      PrintDiagnosticHeader(out, opt, fs, mminfo.pos, DiagnosticClass::INFO, "First method declared here.");
      PrintRangePtr(out, opt, fs, mminfo.pos);
      *out << '\n';
      PrintDiagnosticHeader(out, opt, fs, pminfo.pos, DiagnosticClass::INFO, "Second method declared here.");
      PrintRangePtr(out, opt, fs, pminfo.pos);
    }
  });
}

} // namespace

TypeInfoMap TypeInfoMap::kEmptyTypeInfoMap = TypeInfoMap{{}, TypeId::kError};
TypeInfo TypeInfoMap::kErrorTypeInfo = TypeInfo{{}, TypeKind::CLASS, TypeId::kError, "", "", kFakePos, TypeIdList({}), TypeIdList({}), MethodTable::kErrorMethodTable, FieldTable::kErrorFieldTable, 0};

MethodTable MethodTable::kEmptyMethodTable = MethodTable({}, {}, false);
MethodTable MethodTable::kErrorMethodTable = MethodTable();
MethodInfo MethodTable::kErrorMethodInfo = MethodInfo{kErrorMethodId, TypeId::kError, {}, TypeId::kError, kFakePos, {false, "", TypeIdList({})}, kErrorMethodId};

FieldTable FieldTable::kEmptyFieldTable = FieldTable({}, {});
FieldTable FieldTable::kErrorFieldTable = FieldTable();
FieldInfo FieldTable::kErrorFieldInfo = FieldInfo{kErrorFieldId, TypeId::kError, {}, TypeId::kError, kFakePos, ""};

void PrintMethodSignatureTo(ostream* out, const TypeInfoMap& tinfo_map, const MethodSignature& m_sig) {
  *out << m_sig.name << '(';
  for (int i = 0; i < m_sig.param_types.Size(); ++i) {
    if (i > 0) {
      *out << ", ";
    }
    *out << tinfo_map.LookupTypeName(m_sig.param_types.At(i));
  }
  *out << ")";
}

TypeInfoMapBuilder::TypeInfoMapBuilder(TypeId object_tid, TypeId serializable_tid, TypeId cloneable_tid) : object_tid_(object_tid) {
  // Create array type.
  array_tid_ = TypeId{object_tid_.base, 1};
  PutType(
      array_tid_,
      MakeModifierList(false, false, false),
      ast::TypeKind::CLASS,
      "array", "", kFakePos,
      {object_tid_},
      {serializable_tid, cloneable_tid});
  PutField(
      array_tid_,
      FieldInfo{
        kArrayLengthFieldId,
        array_tid_,
        MakeModifierList(false, false, false),
        TypeId::kInt,
        kFakePos,
        "length"});
}

Error* TypeInfoMapBuilder::MakeConstructorNameError(PosRange pos) const {
  return MakeSimplePosRangeError(pos, "ConstructorNameError", "Constructors must have the same name as its class.");
}

Error* TypeInfoMapBuilder::MakeParentFinalError(const TypeInfo& minfo, const TypeInfo& pinfo) const {
  stringstream msgstream;
  msgstream << "A class may not extend '" << pinfo.name << "', a final class.";
  const string p_msg = "Declared final here.";
  return MakeSimpleMethodTableError(minfo.pos, msgstream.str(), pinfo.pos, p_msg, "ParentFinalError");
}

Error* TypeInfoMapBuilder::MakeDifferingReturnTypeError(const TypeInfo& mtinfo, const MethodInfo& mminfo, const MethodInfo& pminfo) const {
  const string m_msg = "Cannot have methods with overloaded return types.";
  const string p_msg = "Parent method declared here.";
  return MakeResolveMethodTableError(mtinfo, mminfo, pminfo, m_msg, p_msg, "DifferingReturnTypeError");
}

Error* TypeInfoMapBuilder::MakeStaticMethodOverrideError(const TypeInfo& mtinfo, const MethodInfo& mminfo, const MethodInfo& pminfo) const {
  const string m_msg = "A class may not inherit a static method, nor may it override using a static method.";
  const string p_msg = "Parent method declared here.";
  return MakeResolveMethodTableError(mtinfo, mminfo, pminfo, m_msg, p_msg, "StaticMethodOverrideError");
}

Error* TypeInfoMapBuilder::MakeLowerVisibilityError(const TypeInfo& mtinfo, const MethodInfo& mminfo, const MethodInfo& pminfo) const {
  const string m_msg = "A class may not lower the visibility of an inherited method.";
  const string p_msg = "Parent method declared here.";
  return MakeResolveMethodTableError(mtinfo, mminfo, pminfo, m_msg, p_msg, "LowerVisibilityError");
}

Error* TypeInfoMapBuilder::MakeOverrideFinalMethodError(const MethodInfo& minfo, const MethodInfo& pinfo) const {
  const string m_msg = "A class may not override a final method.";
  const string p_msg = "Final method declared here.";
  return MakeSimpleMethodTableError(minfo.pos, m_msg, pinfo.pos, p_msg, "OverrideFinalMethodError");
}

Error* TypeInfoMapBuilder::MakeParentClassEmptyConstructorError(const TypeInfo& minfo, const TypeInfo& pinfo) const {
  const string p_msg = "An inherited class must have a zero-argument constructor.";
  const string m_msg = "Child class declared here.";
  return MakeSimpleMethodTableError(pinfo.pos, p_msg, minfo.pos, m_msg, "ParentClassEmptyConstructorError");
}

Error* TypeInfoMapBuilder::MakeNeedAbstractClassError(const TypeInfo& tinfo, const MethodTable::MethodSignatureMap& method_map) const {
  return MakeError([=](ostream* out, const OutputOptions& opt, const base::FileSet* fs) {
    if (opt.simple) {
      *out << "NeedAbstractClassError: [" << tinfo.pos << ']';
      return;
    }
    const string m_msg = "A class containing abstract methods must be abstract.";
    const string l_msg = "Abstract method declared here.";

    PrintDiagnosticHeader(out, opt, fs, tinfo.pos, DiagnosticClass::ERROR, m_msg);
    PrintRangePtr(out, opt, fs, tinfo.pos);
    for (auto sig_pair = method_map.cbegin(); sig_pair != method_map.cend(); ++sig_pair) {
      const MethodInfo& minfo = sig_pair->second;
      if (!minfo.mods.HasModifier(ABSTRACT)) {
        continue;
      }
      *out << '\n';
      PrintDiagnosticHeader(out, opt, fs, minfo.pos, DiagnosticClass::INFO, l_msg);
      PrintRangePtr(out, opt, fs, minfo.pos);
    }
  });
}

Error* TypeInfoMapBuilder::MakeExtendsCycleError(const vector<TypeInfo>& cycle) const {
  return MakeError([=](ostream* out, const OutputOptions& opt, const base::FileSet* fs) {
    CHECK(cycle.size() > 1);
    if (opt.simple) {
      *out << "ExtendsCycleError{";
      for (uint i = 0; i < cycle.size() - 1; ++i) {
        *out << cycle.at(i).name << "->" << cycle.at(i+1).name << ",";
      }
      *out << '}';
      return;
    }

    stringstream firstmsg;
    firstmsg << "Illegal extends-cycle starts here; " << cycle.at(0).name << " extends "
        << cycle.at(1).name << ".";

    PrintDiagnosticHeader(out, opt, fs, cycle.at(0).pos, DiagnosticClass::ERROR, firstmsg.str());
    PrintRangePtr(out, opt, fs, cycle.at(0).pos);
    for (uint i = 1; i < cycle.size() - 1; ++i) {
      *out << '\n';

      TypeInfo prev = cycle.at(i);
      TypeInfo cur = cycle.at(i + 1);

      stringstream msg;
      msg << prev.name << " extends " << cur.name << ".";
      PrintDiagnosticHeader(out, opt, fs, prev.pos, DiagnosticClass::INFO, msg.str());
      PrintRangePtr(out, opt, fs, prev.pos);
    }
  });
}

TypeIdList Concat(const initializer_list<TypeIdList>& types) {
  vector<TypeId> typeids;
  for (const auto& list : types) {
    for (int i = 0; i < list.Size(); ++i) {
      typeids.push_back(list.At(i));
    }
  }
  return TypeIdList(typeids);
}

MethodInfo FixMods(const TypeInfo& tinfo, const MethodInfo& minfo) {
  if (tinfo.kind == TypeKind::CLASS) {
    return minfo;
  }

  MethodInfo ret_minfo = minfo;
  ret_minfo.mods.AddModifier(kPublic);
  ret_minfo.mods.AddModifier(kAbstract);

  return ret_minfo;
}

MethodTable TypeInfoMapBuilder::MakeResolvedMethodTable(TypeInfo* tinfo, const MethodTable::MethodSignatureMap& good_methods, const set<string>& bad_methods, bool has_bad_constructor, const map<TypeId, TypeInfo>& sofar, const set<TypeId>& bad_types, set<TypeId>* new_bad_types, ErrorList* out) {
  MethodTable::MethodSignatureMap new_good_methods(good_methods);
  set<string> new_bad_methods(bad_methods);

  TypeIdList parents = Concat({tinfo->extends, tinfo->implements});

  for (int i = 0; i < parents.Size(); ++i) {
    const auto info_pair = sofar.find(parents.At(i));
    if (info_pair == sofar.cend()) {
      continue;
    }
    const TypeInfo& pinfo = info_pair->second;

    // Early return if any of our parents are broken.
    if (bad_types.count(pinfo.type) == 1 || pinfo.methods.all_blacklisted_) {
      return MethodTable::kErrorMethodTable;
    }

    // We cannot inherit from a parent that is declared final.
    if (pinfo.mods.HasModifier(FINAL)) {
      out->Append(MakeParentFinalError(*tinfo, pinfo));
      continue;
    }

    bool has_empty_constructor = false;

    for (const auto& psig_pair : pinfo.methods.method_signatures_) {
      const MethodSignature& psig = psig_pair.first;
      const MethodInfo& pminfo = psig_pair.second;

      // Skip constructors since they are not inherited.
      if (psig.is_constructor) {
        if (psig.param_types.Size() == 0) {
          has_empty_constructor = true;
        }
        continue;
      }

      // Already blacklisted in child.
      if (new_bad_methods.count(psig.name) == 1) {
        continue;
      }

      auto msig_pair = new_good_methods.find(psig);

      // No corresponding method in child so add it to our map.
      if (msig_pair == new_good_methods.end()) {
        new_good_methods.insert({psig, pminfo});
        continue;
      }

      const MethodInfo& mminfo = msig_pair->second;

      // If inherited method is exactly the same one (diamond) then continue.
      if (pminfo.mid == mminfo.mid) {
        continue;
      }

      // We cannot inherit methods of the same signature but differing return
      // types.
      if (pminfo.return_type != mminfo.return_type) {
        out->Append(MakeDifferingReturnTypeError(*tinfo, mminfo, pminfo));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }

      // Inheriting methods that are static or overriding with a static method
      // are not allowed.
      if (pminfo.mods.HasModifier(lexer::STATIC) || mminfo.mods.HasModifier(lexer::STATIC)) {
        out->Append(MakeStaticMethodOverrideError(*tinfo, mminfo, pminfo));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }

      CHECK(!pminfo.mods.HasModifier(NATIVE));
      CHECK(!mminfo.mods.HasModifier(NATIVE));

      bool inherited_both_abstract = (mminfo.class_type != tinfo->type && pminfo.mods.HasModifier(ABSTRACT) && mminfo.mods.HasModifier(ABSTRACT));
      // We can't lower visibility of inherited methods if we have an implementation.
      if (pminfo.mods.HasModifier(PUBLIC) && mminfo.mods.HasModifier(PROTECTED) && !inherited_both_abstract) {
        out->Append(MakeLowerVisibilityError(*tinfo, mminfo, pminfo));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }
      bool is_protected = mminfo.mods.HasModifier(PROTECTED);

      // Promote to public accessibility if we inherited a protected abstract
      // method and inherit a public abstract method from an interface.
      if (inherited_both_abstract && pminfo.mods.HasModifier(PUBLIC)) {
        is_protected = false;
      }

      // We can't override final methods.
      if (pminfo.mods.HasModifier(FINAL)) {
        out->Append(MakeOverrideFinalMethodError(mminfo, pminfo));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }
      bool is_final = mminfo.mods.HasModifier(FINAL);
      bool is_abstract = mminfo.mods.HasModifier(ABSTRACT);

      MethodInfo final_mminfo = MethodInfo {
        mminfo.mid,
        mminfo.class_type,
        MakeModifierList(is_protected, is_final, is_abstract),
        mminfo.return_type,
        mminfo.pos,
        mminfo.signature,
        pinfo.kind == TypeKind::CLASS ? pminfo.mid : mminfo.parent_mid,
      };

      msig_pair->second = final_mminfo;
    }

    if (!has_empty_constructor &&
        pinfo.kind != TypeKind::INTERFACE &&
        new_bad_types->count(pinfo.type) == 0) {
      out->Append(MakeParentClassEmptyConstructorError(*tinfo, pinfo));
      new_bad_types->insert(pinfo.type);
    }

    // Union sets of disallowed names from the parent.
    new_bad_methods.insert(pinfo.methods.bad_methods_.begin(), pinfo.methods.bad_methods_.end());
  }

  // If we have abstract methods, we must also be abstract.
  {
    auto is_abstract_method = [](pair<MethodSignature, MethodInfo> pair) {
      return pair.second.mods.HasModifier(ABSTRACT);
    };
    bool has_abstract = std::any_of(new_good_methods.cbegin(), new_good_methods.cend(), is_abstract_method);

    if (has_abstract && tinfo->kind != TypeKind::INTERFACE && !tinfo->mods.HasModifier(ABSTRACT)) {
      out->Append(MakeNeedAbstractClassError(*tinfo, new_good_methods));
    }
  }

  return MethodTable(new_good_methods, new_bad_methods, has_bad_constructor);
}

// Builds valid MethodTables for a TypeInfo. Emits errors if methods for the
// type are invalid.
void TypeInfoMapBuilder::BuildMethodTable(MInfoIter begin, MInfoIter end, TypeInfo* tinfo, MethodId* cur_mid, const map<TypeId, TypeInfo>& sofar, const set<TypeId>& bad_types, set<TypeId>* new_bad_types, ErrorList* out) {
  // Sort all MethodInfo to cluster them by signature.
  auto lt_cmp = [](const MethodInfo& lhs, const MethodInfo& rhs) {
    return lhs.signature < rhs.signature;
  };
  stable_sort(begin, end, lt_cmp);

  MethodTable::MethodSignatureMap good_methods;
  set<string> bad_methods;
  bool has_bad_constructor = false;

  // Build MethodTable ignoring parent methods.
  {
    auto eq_cmp = [&lt_cmp](const MethodInfo& lhs, const MethodInfo& rhs) {
      return !lt_cmp(lhs, rhs) && !lt_cmp(rhs, lhs);
    };

    auto cb = [&](MInfoCIter lbegin, MInfoCIter lend, i64 ndups) {
      // Make sure constructors are named the same as the class.
      for (auto cur = lbegin; cur != lend; ++cur) {
        if (cur->signature.is_constructor && cur->signature.name != tinfo->name) {
          out->Append(MakeConstructorNameError(cur->pos));
          has_bad_constructor = true;
        }
      }

      // Add non-duplicate MethodInfo to the MethodTable.
      if (ndups == 1) {
        MethodInfo new_info = *lbegin;
        new_info.mid = *cur_mid;
        good_methods.insert({new_info.signature, new_info});
        ++(*cur_mid);
        return;
      }

      // Emit error for duped methods.
      CHECK(ndups > 1);

      vector<PosRange> defs;
      for (auto cur = lbegin; cur != lend; ++cur) {
        defs.push_back(cur->pos);
      }
      stringstream msgstream;
      if (lbegin->signature.is_constructor) {
        msgstream << "Constructor";
      } else {
        msgstream << "Method";
      }
      msgstream << " '" << lbegin->signature.name << "' was declared multiple times.";
      out->Append(MakeDuplicateDefinitionError(defs, msgstream.str(), lbegin->signature.name));
      bad_methods.insert(lbegin->signature.name);
    };

    FindEqualRanges(begin, end, eq_cmp, cb);
  }

  tinfo->methods = MakeResolvedMethodTable(tinfo, good_methods, bad_methods, has_bad_constructor, sofar, bad_types, new_bad_types, out);
}

// Builds valid FieldTables for a TypeInfo. Emits errors if fields for the type are invalid.
void TypeInfoMapBuilder::BuildFieldTable(FInfoIter begin, FInfoIter end, TypeInfo* tinfo, FieldId* cur_fid, const map<TypeId, TypeInfo>& sofar, ErrorList* out) {
  // Assign field ids in source-code order.
  for (auto cur = begin; cur != end; ++cur) {
    // If we've already assigned a field id to this field, then don't overwrite
    // that assignment.
    if (cur->fid != kErrorFieldId) {
      continue;
    }

    cur->fid = *cur_fid;
    ++(*cur_fid);
  }

  // Sort all FieldInfo to cluster them by name.
  auto lt_cmp = [](const FieldInfo& lhs, const FieldInfo& rhs) {
    return lhs.name < rhs.name;
  };

  stable_sort(begin, end, lt_cmp);

  FieldTable::FieldNameMap good_fields;
  set<string> bad_fields;

  // Build FieldTable ignoring parent fields.
  {
    auto eq_cmp = [&lt_cmp](const FieldInfo& lhs, const FieldInfo& rhs) {
      return !lt_cmp(lhs, rhs) && !lt_cmp(rhs, lhs);
    };

    auto cb = [&](FInfoCIter lbegin, FInfoCIter lend, i64 ndups) {
      // Add non-duplicate FieldInfo to the FieldTable.
      if (ndups == 1) {
        good_fields.insert({lbegin->name, *lbegin});
        return;
      }

      // Emit error for duped fields.
      CHECK(ndups > 1);

      vector<PosRange> defs;
      for (auto cur = lbegin; cur != lend; ++cur) {
        defs.push_back(cur->pos);
      }
      stringstream msgstream;
      msgstream << "Field '" << lbegin->name << "' was declared multiple times.";
      out->Append(MakeDuplicateDefinitionError(defs, msgstream.str(), lbegin->name));
      bad_fields.insert(lbegin->name);
    };

    FindEqualRanges(begin, end, eq_cmp, cb);
  }

  FieldTable::FieldNameMap new_good_fields(good_fields);
  set<string> new_bad_fields(bad_fields);

  TypeIdList parents = Concat({tinfo->extends, tinfo->implements});

  for (int i = 0; i < parents.Size(); ++i) {
    const auto info_pair = sofar.find(parents.At(i));
    if (info_pair == sofar.end()) {
      continue;
    }
    const TypeInfo& pinfo = info_pair->second;

    // Early return if any of our parents are broken.
    if (pinfo.fields.all_blacklisted_) {
      tinfo->fields = FieldTable::kErrorFieldTable;
      return;
    }

    for (const auto& pname_pair : pinfo.fields.field_names_) {
      const string& pname = pname_pair.first;
      const FieldInfo pfinfo = pname_pair.second;

      // Already blacklisted in child.
      if (new_bad_fields.count(pname) == 1) {
        continue;
      }

      auto mname_pair = new_good_fields.find(pname);

      // No corresponding field in child so add it to our map.
      if (mname_pair == new_good_fields.end()) {
        new_good_fields.insert({pname, pfinfo});
        continue;
      }
    }

    // Union sets of disallowed names from the parent.
    new_bad_fields.insert(pinfo.fields.bad_fields_.begin(), pinfo.fields.bad_fields_.end());
  }

  tinfo->fields = FieldTable(new_good_fields, new_bad_fields);
}

TypeInfoMap TypeInfoMapBuilder::Build(base::ErrorList* out) {
  map<TypeId, TypeInfo> typeinfo;
  vector<TypeId> all_types;
  set<TypeId> cycle_bad_types;
  set<TypeId> parent_bad_types;

  for (const auto& entry : type_entries_) {
    typeinfo.insert({entry.type, entry});
    all_types.push_back(entry.type);
  }

  ValidateExtendsImplementsGraph(&typeinfo, &cycle_bad_types, out);

  // Sort TypeId vector by the topological ordering of the types.
  {
    auto t_cmp = [&typeinfo](TypeId lhs, TypeId rhs) {
      return typeinfo.at(lhs).top_sort_index < typeinfo.at(rhs).top_sort_index;
    };
    stable_sort(all_types.begin(), all_types.end(), t_cmp);
  }

  // Populate MethodTables and FieldTables for each TypeInfo.
  {
    MethodId cur_mid = kFirstMethodId;
    FieldId cur_fid = kFirstFieldId;

    for (auto type_id : all_types) {
      if (cycle_bad_types.count(type_id) == 1) {
        typeinfo.at(type_id) = TypeInfoMap::kErrorTypeInfo;
        continue;
      }

      TypeInfo* tinfo = &typeinfo.at(type_id);
      {
        auto iter_pair = method_entries_.equal_range(type_id);
        vector<MethodInfo> methods;
        for (auto cur = iter_pair.first; cur != iter_pair.second; ++cur) {
          methods.push_back(FixMods(*tinfo, cur->second));
        }

        BuildMethodTable(methods.begin(), methods.end(), tinfo, &cur_mid, typeinfo, cycle_bad_types, &parent_bad_types, out);
      }


      {
        auto iter_pair = field_entries_.equal_range(type_id);
        vector<FieldInfo> fields;
        for (auto cur = iter_pair.first; cur != iter_pair.second; ++cur) {
          fields.push_back(cur->second);
        }

        BuildFieldTable(fields.begin(), fields.end(), tinfo, &cur_fid, typeinfo, out);
      }
    }
  }

  for (auto type_id : parent_bad_types) {
    typeinfo.at(type_id) = TypeInfoMap::kErrorTypeInfo;
  }

  return TypeInfoMap(typeinfo, array_tid_);
}

void TypeInfoMapBuilder::ValidateExtendsImplementsGraph(map<TypeId, TypeInfo>* types, set<TypeId>* bad, ErrorList* errors) {
  using IdInfoMap = map<TypeId, TypeInfo>;

  // Bind a reference to make the code more readable.
  IdInfoMap& all_types = *types;
  set<TypeId>& bad_types = *bad;

  // Ensure that we blacklist any classes that introduce invalid edges into the
  // graph.
  PruneInvalidGraphEdges(all_types, &bad_types, errors);

  // Make every class and interface extend Object.
  IntroduceImplicitGraphEdges(bad_types, &all_types);

  // Now build a combined graph of edges.
  multimap<TypeId, TypeId> edges;
  for (const auto& type_pair : all_types) {
    const TypeId type = type_pair.first;
    const TypeInfo& typeinfo = type_pair.second;

    if (bad_types.count(type) == 1) {
      continue;
    }

    TypeIdList parents = Concat({typeinfo.extends, typeinfo.implements});
    for (int i = 0; i < parents.Size(); ++i) {
      edges.insert({type, parents.At(i)});
    }
  }

  // Verify that the combined graph is acyclic.
  {
    auto cycle_cb = [&](const vector<TypeId>& cycle) {
      vector<TypeInfo> infos;
      for (const auto& tid : cycle) {
        infos.push_back(all_types.at(tid));
      }
      errors->Append(MakeExtendsCycleError(infos));
    };

    vector<TypeId> topsort = VerifyAcyclicGraph(edges, &bad_types, cycle_cb);
    for (uint i = 0; i < topsort.size(); ++i) {
      auto type_iter = all_types.find(topsort.at(i));
      if (type_iter == all_types.end()) {
        continue;
      }

      type_iter->second.top_sort_index = i;
    }
  }
}

void TypeInfoMapBuilder::PruneInvalidGraphEdges(const map<TypeId, TypeInfo>& all_types, set<TypeId>* bad_types, ErrorList* errors) {
  // Blacklists child if parent doesn't exist in all_types, or parent's kind
  // doesn't match expected_parent_kind.
  auto match_relationship = [&](TypeId parent, TypeId child, TypeKind expected_parent_kind) {
    auto parent_iter = all_types.find(parent);
    if (parent_iter == all_types.end()) {
      bad_types->insert(child);
      return false;
    }

    if (parent_iter->second.kind != expected_parent_kind) {
      bad_types->insert(child);
      return false;
    }

    return true;
  };

  for (const auto& type_pair : all_types) {
    const TypeId type = type_pair.first;
    const TypeInfo& typeinfo = type_pair.second;

    if (typeinfo.kind == TypeKind::INTERFACE) {
      // Previous passes validate that interfaces cannot implement anything.
      CHECK(typeinfo.implements.Size() == 0);

      // An interface can only extend other interfaces.
      set<TypeId> already_extended;
      for (int i = 0; i < typeinfo.extends.Size(); ++i) {
        TypeId extends_tid = typeinfo.extends.At(i);
        auto is_duplicate = already_extended.insert(extends_tid);

        if (all_types.count(extends_tid) == 0) {
          continue;
        }

        if (!match_relationship(extends_tid, type, TypeKind::INTERFACE)) {
          errors->Append(MakeInterfaceExtendsClassError(typeinfo.pos, all_types.at(extends_tid).name));
          bad_types->insert(type);
        }
        if (!is_duplicate.second) {
          errors->Append(MakeDuplicateInheritanceError(true, typeinfo.pos, typeinfo.type, extends_tid));
          bad_types->insert(type);
          break;
        }
      }
      continue;
    }

    CHECK(typeinfo.kind == TypeKind::CLASS);
    CHECK(typeinfo.extends.Size() <= 1);
    if (typeinfo.extends.Size() == 1) {
      TypeId parent_tid = typeinfo.extends.At(0);
      if (!match_relationship(parent_tid, type, TypeKind::CLASS)) {
        errors->Append(MakeClassExtendsInterfaceError(typeinfo.pos, all_types.at(parent_tid).name));
        bad_types->insert(type);
      }
    }
    set<TypeId> already_implemented;
    for (int i = 0; i < typeinfo.implements.Size(); ++i) {
      TypeId implement_tid = typeinfo.implements.At(i);
      auto is_duplicate = already_implemented.insert(implement_tid);

      if (bad_types->count(implement_tid) != 0 || all_types.count(implement_tid) == 0) {
        continue;
      }

      if (!match_relationship(implement_tid, type, TypeKind::INTERFACE)) {
        errors->Append(MakeClassImplementsClassError(typeinfo.pos, all_types.at(implement_tid).name));
        bad_types->insert(type);
      }
      if (!is_duplicate.second) {
        errors->Append(MakeDuplicateInheritanceError(false, typeinfo.pos, typeinfo.type, implement_tid));
        bad_types->insert(type);
        break;
      }
    }
  }
}

void TypeInfoMapBuilder::IntroduceImplicitGraphEdges(const set<TypeId>& bad, map<TypeId, TypeInfo>* types) {
  using IdInfoMap = map<TypeId, TypeInfo>;

  // Bind a reference to make the code more readable.
  IdInfoMap& all_types = *types;

  for (auto& tid_tinfo_iter : all_types) {
    TypeId tid = tid_tinfo_iter.first;
    TypeInfo& tinfo = tid_tinfo_iter.second;

    // Do nothing for already blacklisted types.
    if (bad.count(tid) == 1) {
      continue;
    }

    // We don't insert implicit edges for Object.
    if (tid == object_tid_) {
      continue;
    }

    // If the type is already extending things then do nothing. They'll get
    // the implicit edge indirectly.
    if (tinfo.extends.Size() > 0) {
      continue;
    }

    tinfo.extends = TypeIdList({object_tid_});
  }
}

vector<TypeId> TypeInfoMapBuilder::VerifyAcyclicGraph(const multimap<TypeId, TypeId>& edges, set<TypeId>* bad_types, function<void(const vector<TypeId>& cycle)> cb) {
  // Both of these store different representations of the current recursion
  // path.
  set<TypeId> open;
  vector<TypeId> path;

  // Tracks known values of TypeIds.
  set<TypeId> good;
  set<TypeId>& bad = *bad_types;

  // Stored in top-sorted order.
  vector<TypeId> sorted;

  // Predeclared so that the implementation can recurse.
  function<bool(TypeId)> fn;
  fn = [&](TypeId tid) {
    if (bad.count(tid) == 1) {
      return false;
    }

    if (good.count(tid) == 1) {
      return true;
    }

    // Check if we form a cycle.
    if (open.count(tid) == 1) {
      // Find the start of the cycle.
      auto iter = find(path.begin(), path.end(), tid);
      CHECK(iter != path.end());

      vector<TypeId> cycle(iter, path.end());
      cycle.push_back(tid);

      cb(cycle);
      bad.insert(tid);
      return false;
    }

    open.insert(tid);
    path.push_back(tid);

    auto iter_pair = edges.equal_range(tid);
    bool ok = true;
    for (auto cur = iter_pair.first; cur != iter_pair.second; ++cur) {
      size_t prev_path_size = path.size();
      ok = ok && fn(cur->second);
      CHECK(path.size() == prev_path_size);

      if (!ok) {
        break;
      }
    }

    CHECK(path.size() > 0);
    CHECK(path.back() == tid);
    path.pop_back();
    open.erase(tid);

    if (!ok) {
      bad.insert(tid);
      return false;
    }

    CHECK(ok);
    good.insert(tid);
    sorted.push_back(tid);

    return true;
  };

  for (const auto& type : edges) {
    fn(type.first);
  }

  return sorted;
}

string TypeInfoMap::LookupTypeName(ast::TypeId tid) const {
  CHECK(tid != ast::TypeId::kUnassigned);
  CHECK(tid != ast::TypeId::kError);

  stringstream ss;
  switch (tid.base) {
    case ast::TypeId::kNullBase:  ss << "null"; break;
    case ast::TypeId::kTypeBase:  ss << "type"; break;
    case ast::TypeId::kVoidBase:  ss << "void"; break;
    case ast::TypeId::kBoolBase:  ss << "boolean"; break;
    case ast::TypeId::kByteBase:  ss << "byte"; break;
    case ast::TypeId::kCharBase:  ss << "char"; break;
    case ast::TypeId::kShortBase: ss << "short"; break;
    case ast::TypeId::kIntBase:   ss << "int"; break;
    default: ss << LookupTypeInfo({tid.base, 0}).name;
  }
  for (u64 i = 0; i < tid.ndims; ++i) {
    ss << "[]";
  }
  return ss.str();
}

bool TypeInfoMap::IsAncestor(TypeId child, TypeId ancestor) const {
  auto ancestor_lookup = inherit_map_.find(make_pair(child, ancestor));
  if (ancestor_lookup != inherit_map_.end()) {
    return ancestor_lookup->second;
  }
  bool is_ancestor = IsAncestorRec(child, ancestor);
  inherit_map_.insert({make_pair(child, ancestor), is_ancestor});
  return is_ancestor;
}

bool TypeInfoMap::IsAncestorRec(TypeId child, TypeId ancestor) const {
  const TypeInfo& tinfo = LookupTypeInfo(child);
  if (tinfo.type == ast::TypeId::kError) {
    // If blacklisted, allow any inheritance check.
    return true;
  }
  types::TypeIdList parents = Concat({tinfo.extends, tinfo.implements});
  for (int i = 0; i < parents.Size(); ++i) {
    // If this parent is the ancestor we're looking for, return immediately.
    if (parents.At(i) == ancestor) {
      return true;
    }

    // Recurse using the cached/memoized lookup on our parents.
    if (IsAncestor(parents.At(i), ancestor)) {
      return true;
    }
  }

  return false;
}

bool MethodTable::IsBlacklisted(CallContext ctx, const string& name) const {
  if (all_blacklisted_) {
    return true;
  }
  if (ctx == CallContext::CONSTRUCTOR) {
    return has_bad_constructor_;
  }
  return bad_methods_.count(name) == 1;
}

MethodId MethodTable::ResolveCall(const TypeInfoMap& type_info_map, TypeId caller_type, CallContext ctx, TypeId callee_type, const TypeIdList& params, const string& method_name, PosRange pos, ErrorList* errors) const {
  bool is_constructor = ctx == CallContext::CONSTRUCTOR;
  MethodSignature sig = MethodSignature{is_constructor, method_name, params};
  auto minfo = method_signatures_.find(sig);
  if (minfo == method_signatures_.end()) {
    // Only emit error if this isn't blacklisted.
    if (!IsBlacklisted(ctx, method_name)) {
      errors->Append(MakeUndefinedMethodError(type_info_map, sig, pos));
    }
    return kErrorMethodId;
  }

  if (is_constructor && type_info_map.LookupTypeInfo(callee_type).mods.HasModifier(lexer::Modifier::ABSTRACT)) {
    errors->Append(MakeNewAbstractClassError(pos));
    return kErrorMethodId;
  }

  // Check whether calling context is correct.
  bool is_static = minfo->second.mods.HasModifier(lexer::STATIC);
  if (is_static && ctx != CallContext::STATIC) {
    errors->Append(MakeInstanceMethodOnStaticError(pos));
    return kErrorMethodId;
  } else if (!is_static && ctx == CallContext::STATIC) {
    errors->Append(MakeStaticMethodOnInstanceError(pos));
    return kErrorMethodId;
  }

  // Check permissions.
  if (!IsAccessible(type_info_map, minfo->second.mods, ctx, minfo->second.class_type, caller_type, callee_type)) {
    errors->Append(MakePermissionError(pos, minfo->second.pos));
    return kErrorMethodId;
  }

  return minfo->second.mid;
}

Error* MethodTable::MakePermissionError(PosRange call_pos, PosRange method_pos) const {
  return MakeError([=](ostream* out, const OutputOptions& opt, const base::FileSet* fs) {
    if (opt.simple) {
      *out << "PermissionError: [" << call_pos << ',' << method_pos <<  ']';
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, call_pos, DiagnosticClass::ERROR, "Cannot access protected method from a non-descendant.");
    PrintRangePtr(out, opt, fs, call_pos);
    *out << '\n';
    PrintDiagnosticHeader(out, opt, fs, method_pos, DiagnosticClass::INFO, "Defined here.");
    PrintRangePtr(out, opt, fs, method_pos);
  });
}

Error* MethodTable::MakeUndefinedMethodError(const TypeInfoMap& tinfo_map, const MethodSignature& sig, PosRange pos) const {
  MethodSignatureMap method_signatures(method_signatures_);
  return MakeError([=](ostream* out, const OutputOptions& opt, const base::FileSet* fs) {
    if (opt.simple) {
      *out << "UndefinedMethodError(" << pos << ')';
      return;
    }

    int num_params = sig.param_types.Size();
    {
      stringstream ss;
      ss << "Couldn't find ";
      if (sig.is_constructor) {
        ss << "constructor ";
      } else {
        ss << "method ";
      }
      ss << '\'';
      PrintMethodSignatureTo(&ss, tinfo_map, sig);
      ss << "'.";
      PrintDiagnosticHeader(out, opt, fs, pos, DiagnosticClass::ERROR, ss.str());
      PrintRangePtr(out, opt, fs, pos);
    }

    // Print available methods of the same name if available.
    const MethodSignature find_sig = {sig.is_constructor, sig.name, TypeIdList({})};
    auto cur = method_signatures.lower_bound(find_sig);
    for (; cur != method_signatures.end(); ++cur) {
      const MethodSignature& found_sig = cur->first;
      if (sig.is_constructor != found_sig.is_constructor || sig.name != found_sig.name) {
        break;
      }

      stringstream ss;
      ss << '\'';
      PrintMethodSignatureTo(&ss, tinfo_map, found_sig);
      ss << "' not viable: ";
      int found_num_params = found_sig.param_types.Size();
      if (num_params != found_num_params) {
        ss << "different number of arguments provided, got " << num_params;
        ss << ", need " << found_num_params << '.';
      } else {
        const TypeIdList& m_params = sig.param_types;
        const TypeIdList& found_params = found_sig.param_types;
        int i = 0;
        for (; i < num_params; ++i) {
          if (m_params.At(i) != found_params.At(i)) {
            break;
          }
        }
        CHECK(i != num_params);
        ss << "for argument " << i + 1 << ", got " << tinfo_map.LookupTypeName(m_params.At(i));
        ss << ", need " << tinfo_map.LookupTypeName(found_params.At(i)) << '.';
      }

      *out << '\n';
      PrintDiagnosticHeader(out, opt, fs, cur->second.pos, DiagnosticClass::INFO, ss.str());
      PrintRangePtr(out, opt, fs, cur->second.pos);
    }
  });
}

Error* MethodTable::MakeNewAbstractClassError(PosRange pos) const {
  return MakeSimplePosRangeError(pos, "NewAbstractClassError", "Cannot instantiate abstract class.");
}

Error* MethodTable::MakeInstanceMethodOnStaticError(PosRange pos) const {
  const static string msg = "Cannot call a static method as an instance method.";
  return MakeSimplePosRangeError(pos, "InstanceMethodOnStaticError", msg);
}

Error* MethodTable::MakeStaticMethodOnInstanceError(PosRange pos) const {
  const static string msg = "Cannot call an instance method as a static method.";
  return MakeSimplePosRangeError(pos, "StaticMethodOnInstanceError", msg);
}

FieldId FieldTable::ResolveAccess(const TypeInfoMap& type_info_map, TypeId caller_type, CallContext ctx, TypeId callee_type, string field_name, PosRange pos, ErrorList* errors) const {
  CHECK(ctx == CallContext::INSTANCE || ctx == CallContext::STATIC);
  auto finfo = field_names_.find(field_name);
  if (finfo == field_names_.end()) {
    // Only emit error if this isn't blacklisted.
    if (!all_blacklisted_ && bad_fields_.count(field_name) == 0) {
      errors->Append(MakeUndefinedReferenceError(field_name, pos));
    }
    return kErrorFieldId;
  }

  // Check whether correct calling context.
  bool is_static = finfo->second.mods.HasModifier(lexer::Modifier::STATIC);
  if (is_static && ctx != CallContext::STATIC) {
    errors->Append(MakeStaticFieldOnInstanceError(pos));
    return kErrorFieldId;
  } else if (!is_static && ctx == CallContext::STATIC) {
    errors->Append(MakeInstanceFieldOnStaticError(pos));
    return kErrorFieldId;
  }

  // Check permissions.
  if (!IsAccessible(type_info_map, finfo->second.mods, ctx, finfo->second.class_type, caller_type, callee_type)) {
    errors->Append(MakePermissionError(pos, finfo->second.pos));
    return kErrorFieldId;
  }

  return finfo->second.fid;
}

Error* FieldTable::MakePermissionError(PosRange access_pos, PosRange field_pos) const {
  return MakeError([=](ostream* out, const OutputOptions& opt, const base::FileSet* fs) {
    if (opt.simple) {
      *out << "PermissionError(" << field_pos << ")";;
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, access_pos, DiagnosticClass::ERROR, "Cannot access protected field from a non-descendant.");
    PrintRangePtr(out, opt, fs, access_pos);
    *out << '\n';
    PrintDiagnosticHeader(out, opt, fs, field_pos, DiagnosticClass::INFO, "Defined here.");
    PrintRangePtr(out, opt, fs, field_pos);
  });
}

Error* FieldTable::MakeUndefinedReferenceError(const string& name, PosRange pos) const {
  stringstream ss;
  ss << "Undefined reference to '";
  ss << name;
  ss << '\'';
  return MakeSimplePosRangeError(pos, "UndefinedReferenceError", ss.str());
}

Error* FieldTable::MakeInstanceFieldOnStaticError(PosRange pos) const {
  const static string msg = "Cannot access an instance field without an instance.";
  return MakeSimplePosRangeError(pos, "InstanceFieldOnStaticError", msg);
}

Error* FieldTable::MakeStaticFieldOnInstanceError(PosRange pos) const {
  const static string msg = "Cannot access a static field as an instance field.";
  return MakeSimplePosRangeError(pos, "StaticFieldOnInstanceError", msg);
}

} // namespace types
