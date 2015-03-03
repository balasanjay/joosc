#include "types/type_info_map.h"

#include "ast/ast.h"
#include "base/algorithm.h"
#include "lexer/lexer.h"
#include "types/types_internal.h"

using std::function;
using std::initializer_list;
using std::ostream;
using std::tie;

using ast::ModifierList;
using ast::TypeId;
using ast::TypeKind;
using base::DiagnosticClass;
using base::Error;
using base::ErrorList;
using base::FileSet;
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

static PosRange kFakePos(-1, -1, -1);
static Token kPublic(K_PUBLIC, kFakePos);
static Token kProtected(K_PROTECTED, kFakePos);
static Token kFinal(K_FINAL, kFakePos);
static Token kAbstract(K_ABSTRACT, kFakePos);

Error* MakeInterfaceExtendsClassError(const FileSet* fs, PosRange pos, const string& parent_class) {
  string msg = "An interface may not extend '" + parent_class + "', a class.";
  return MakeSimplePosRangeError(fs, pos, "InterfaceExtendsClassError", msg);
}

Error* MakeClassImplementsClassError(const FileSet* fs, PosRange pos, const string& parent_class) {
  string msg = "A class may not implement '" + parent_class + "', a class.";
  return MakeSimplePosRangeError(fs, pos, "ClassImplementsClassError", msg);
}

Error* MakeClassExtendsInterfaceError(const FileSet* fs, PosRange pos, const string& parent_iface) {
  string msg = "A class may not extend '" + parent_iface + "', an interface.";
  return MakeSimplePosRangeError(fs, pos, "ClassExtendInterfaceError", msg);
}

Error* MakeResolveMethodTableError(const FileSet* fs, PosRange m_pos, const string& m_string, PosRange p_pos, const string& p_string, const string& error_name) {
  return MakeError([=](ostream* out, const OutputOptions& opt) {
    if (opt.simple) {
      *out << error_name;
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, m_pos, DiagnosticClass::ERROR, m_string);
    PrintRangePtr(out, opt, fs, m_pos);
    *out << '\n';
    PrintDiagnosticHeader(out, opt, fs, p_pos, DiagnosticClass::INFO, p_string);
    PrintRangePtr(out, opt, fs, p_pos);
  });
}

} // namespace

ModifierList MakeModifierList(bool is_protected, bool is_final, bool is_abstract) {

  ModifierList mods;

  if (is_protected) {
    mods.AddModifier(kProtected);
  } else {
    mods.AddModifier(kPublic);
  }

  if (is_final) {
    mods.AddModifier(kFinal);
  }

  if (is_abstract) {
    mods.AddModifier(kAbstract);
  }

  return mods;
}

TypeInfo TypeInfoMap::kErrorTypeInfo = TypeInfo{{}, TypeKind::CLASS, TypeId::kError, "", kFakePos, TypeIdList({}), TypeIdList({}), MethodTable::kErrorMethodTable, FieldTable::kErrorFieldTable, 0};

// TODO: Empty filesets are no.
MethodTable MethodTable::kEmptyMethodTable = MethodTable(&base::FileSet::Empty(), {}, {}, false);
MethodTable MethodTable::kErrorMethodTable = MethodTable();
MethodInfo MethodTable::kErrorMethodInfo = MethodInfo{kErrorMethodId, TypeId::kError, {}, TypeId::kError, kFakePos, {false, "", TypeIdList({})}};

FieldTable FieldTable::kEmptyFieldTable = FieldTable(&base::FileSet::Empty(), {}, {});
FieldTable FieldTable::kErrorFieldTable = FieldTable();
FieldInfo FieldTable::kErrorFieldInfo = FieldInfo{kErrorFieldId, TypeId::kError, {}, TypeId::kError, kFakePos, ""};

Error* TypeInfoMapBuilder::MakeConstructorNameError(PosRange pos) const {
  return MakeSimplePosRangeError(fs_, pos, "ConstructorNameError", "Constructors must have the same name as its class.");
}

Error* TypeInfoMapBuilder::MakeParentFinalError(const TypeInfo& minfo, const TypeInfo& pinfo) const {
  stringstream msgstream;
  msgstream << "A class may not extend '" << pinfo.name << "', a final class.";
  const string p_msg = "Declared final here.";
  return MakeResolveMethodTableError(fs_, minfo.pos, msgstream.str(), pinfo.pos, p_msg, "ParentFinalError");
}

Error* TypeInfoMapBuilder::MakeDifferingReturnTypeError(const TypeInfo& mtinfo, const MethodInfo& mminfo, const MethodInfo& pminfo) const {
  const FileSet* fs = fs_;
  return MakeError([=](ostream* out, const OutputOptions& opt) {
    if (opt.simple) {
      *out << "DifferingReturnTypeError";
      return;
    }

    const string message = "Cannot have methods with overloaded return types.";
    bool is_self_method = (mtinfo.type == mminfo.class_type);
    PosRange m_pos = is_self_method ? mminfo.pos : mtinfo.pos;

    PrintDiagnosticHeader(out, opt, fs, m_pos, DiagnosticClass::ERROR, message);
    PrintRangePtr(out, opt, fs, m_pos);
    *out << '\n';
    if (is_self_method) {
      PrintDiagnosticHeader(out, opt, fs, pminfo.pos, DiagnosticClass::INFO, "Parent method declared here.");
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
Error* TypeInfoMapBuilder::MakeStaticMethodOverrideError(const MethodInfo& minfo, const MethodInfo& pinfo) const {
  const string m_msg = "A class may not inherit a static method, nor may it override using a static method.";
  const string p_msg = "Parent method declared here.";
  return MakeResolveMethodTableError(fs_, minfo.pos, m_msg, pinfo.pos, p_msg, "StaticMethodOverrideError");
}

Error* TypeInfoMapBuilder::MakeLowerVisibilityError(const MethodInfo& minfo, const MethodInfo& pinfo) const {
  const string m_msg = "A class may not lower the visibility of an inherited method.";
  const string p_msg = "Parent method declared here.";
  return MakeResolveMethodTableError(fs_, minfo.pos, m_msg, pinfo.pos, p_msg, "LowerVisibilityError");
}

Error* TypeInfoMapBuilder::MakeOverrideFinalMethodError(const MethodInfo& minfo, const MethodInfo& pinfo) const {
  const string m_msg = "A class may not override a final method.";
  const string p_msg = "Final method declared here.";
  return MakeResolveMethodTableError(fs_, minfo.pos, m_msg, pinfo.pos, p_msg, "OverrideFinalMethodError");
}

Error* TypeInfoMapBuilder::MakeParentClassEmptyConstructorError(const TypeInfo& minfo, const TypeInfo& pinfo) const {
  const string p_msg = "An inherited class must have a zero-argument constructor.";
  const string m_msg = "Child class declared here.";
  return MakeResolveMethodTableError(fs_, pinfo.pos, p_msg, minfo.pos, m_msg, "ParentClassEmptyConstructorError");
}

Error* TypeInfoMapBuilder::MakeNeedAbstractClassError(const TypeInfo& tinfo, const MethodTable::MethodSignatureMap& method_map) const {
  const FileSet* fs = fs_;
  return MakeError([=](ostream* out, const OutputOptions& opt) {
    if (opt.simple) {
      *out << "NeedAbstractClassError";
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
  const FileSet* fs = fs_;
  return MakeError([=](ostream* out, const OutputOptions& opt) {
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
      // TODO: Point to class and two methods.
      if (pminfo.mods.HasModifier(lexer::STATIC) || mminfo.mods.HasModifier(lexer::STATIC)) {
        out->Append(MakeStaticMethodOverrideError(mminfo, pminfo));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }

      CHECK(!pminfo.mods.HasModifier(NATIVE));
      CHECK(!mminfo.mods.HasModifier(NATIVE));

      // We can't lower visibility of inherited methods.
      // TODO: Point to class and two methods.
      if (pminfo.mods.HasModifier(PUBLIC) && mminfo.mods.HasModifier(PROTECTED)) {
        out->Append(MakeLowerVisibilityError(mminfo, pminfo));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }
      bool is_protected = mminfo.mods.HasModifier(PROTECTED);

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
        mminfo.signature
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

  return MethodTable(fs_, new_good_methods, new_bad_methods, has_bad_constructor);
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
  const FileSet* fs = fs_;

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
      out->Append(MakeDuplicateDefinitionError(fs, defs, msgstream.str(), lbegin->signature.name));
      bad_methods.insert(lbegin->signature.name);
    };

    FindEqualRanges(begin, end, eq_cmp, cb);
  }

  tinfo->methods = MakeResolvedMethodTable(tinfo, good_methods, bad_methods, has_bad_constructor, sofar, bad_types, new_bad_types, out);
}

// Builds valid FieldTables for a TypeInfo. Emits errors if fields for the type are invalid.
void TypeInfoMapBuilder::BuildFieldTable(FInfoIter begin, FInfoIter end, TypeInfo* tinfo, FieldId* cur_fid, const map<TypeId, TypeInfo>& sofar, ErrorList* out) {
  // Sort all FieldInfo to cluster them by name.
  auto lt_cmp = [](const FieldInfo& lhs, const FieldInfo& rhs) {
    return lhs.name < rhs.name;
  };
  // TODO: Assign field ids in source code order, not lexicographical order.
  stable_sort(begin, end, lt_cmp);

  FieldTable::FieldNameMap good_fields;
  set<string> bad_fields;

  const FileSet* fs = fs_;

  // Build FieldTable ignoring parent fields.
  {
    auto eq_cmp = [&lt_cmp](const FieldInfo& lhs, const FieldInfo& rhs) {
      return !lt_cmp(lhs, rhs) && !lt_cmp(rhs, lhs);
    };

    auto cb = [&](FInfoCIter lbegin, FInfoCIter lend, i64 ndups) {
      // Add non-duplicate FieldInfo to the FieldTable.
      if (ndups == 1) {
        FieldInfo new_info = *lbegin;
        new_info.fid = *cur_fid;
        good_fields.insert({new_info.name, new_info});
        ++(*cur_fid);
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
      out->Append(MakeDuplicateDefinitionError(fs, defs, msgstream.str(), lbegin->name));
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

  tinfo->fields = FieldTable(fs, new_good_fields, new_bad_fields);
}

TypeInfoMap TypeInfoMapBuilder::Build(const TypeSet& typeset, base::ErrorList* out) {
  map<TypeId, TypeInfo> typeinfo;
  vector<TypeId> all_types;
  set<TypeId> cycle_bad_types;
  set<TypeId> parent_bad_types;

  for (const auto& entry : type_entries_) {
    typeinfo.insert({entry.type, entry});
    all_types.push_back(entry.type);
  }

  ValidateExtendsImplementsGraph(typeset, &typeinfo, &cycle_bad_types, out);

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

  return TypeInfoMap(fs_, typeinfo);
}

void TypeInfoMapBuilder::ValidateExtendsImplementsGraph(const TypeSet& typeset, map<TypeId, TypeInfo>* types, set<TypeId>* bad, ErrorList* errors) {
  using IdInfoMap = map<TypeId, TypeInfo>;

  // Bind a reference to make the code more readable.
  IdInfoMap& all_types = *types;
  set<TypeId>& bad_types = *bad;

  // Ensure that we blacklist any classes that introduce invalid edges into the
  // graph.
  PruneInvalidGraphEdges(all_types, &bad_types, errors);

  // Make every class and interface extend Object.
  IntroduceImplicitGraphEdges(typeset, bad_types, &all_types);

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
  const FileSet* fs = fs_;

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
        if (!is_duplicate.second) {
          errors->Append(MakeDuplicateInheritanceError(fs, true, typeinfo.pos, typeinfo.type, extends_tid));
          bad_types->insert(type);
        }
        if (!match_relationship(extends_tid, type, TypeKind::INTERFACE)) {
          errors->Append(MakeInterfaceExtendsClassError(fs, typeinfo.pos, all_types.at(extends_tid).name));
          bad_types->insert(type);
        }
      }
      continue;
    }

    CHECK(typeinfo.kind == TypeKind::CLASS);
    CHECK(typeinfo.extends.Size() <= 1);
    if (typeinfo.extends.Size() == 1) {
      TypeId parent_tid = typeinfo.extends.At(0);
      if (!match_relationship(parent_tid, type, TypeKind::CLASS)) {
        errors->Append(MakeClassExtendsInterfaceError(fs, typeinfo.pos, all_types.at(parent_tid).name));
        bad_types->insert(type);
      }
    }
    set<TypeId> already_implemented;
    for (int i = 0; i < typeinfo.implements.Size(); ++i) {
      TypeId implement_tid = typeinfo.implements.At(i);
      auto is_duplicate = already_implemented.insert(implement_tid);

      // TODO: don't do this.
      if (all_types.count(implement_tid) == 0) {
        continue;
      }

      if (!is_duplicate.second) {
        errors->Append(MakeDuplicateInheritanceError(fs, false, typeinfo.pos, typeinfo.type, implement_tid));
        bad_types->insert(type);
      }
      if (!match_relationship(implement_tid, type, TypeKind::INTERFACE)) {
        errors->Append(MakeClassImplementsClassError(fs, typeinfo.pos, all_types.at(implement_tid).name));
        bad_types->insert(type);
      }
    }
  }
}

void TypeInfoMapBuilder::IntroduceImplicitGraphEdges(const TypeSet& typeset, const set<TypeId>& bad, map<TypeId, TypeInfo>* types) {
  using IdInfoMap = map<TypeId, TypeInfo>;

  // Bind a reference to make the code more readable.
  IdInfoMap& all_types = *types;

  TypeId object = typeset.TryGet("java.lang.Object");
  CHECK(object.IsValid());

  for (auto& tid_tinfo_iter : all_types) {
    TypeId tid = tid_tinfo_iter.first;
    TypeInfo& tinfo = tid_tinfo_iter.second;

    // Do nothing for already blacklisted types.
    if (bad.count(tid) == 1) {
      continue;
    }

    // We don't insert implicit edges for Object.
    if (tid == object) {
      // TODO: validate that object has no fields.
      continue;
    }

    // If the type is already extending things then do nothing. They'll get
    // the implicit edge indirectly.
    if (tinfo.extends.Size() > 0) {
      continue;
    }

    tinfo.extends = TypeIdList({object});
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

MethodId MethodTable::ResolveCall(TypeId callerType, CallContext ctx, const TypeIdList& params, const string& method_name, PosRange pos, ErrorList* errors) const {
  // TODO: More things.
  MethodSignature sig = MethodSignature{(ctx == CallContext::CONSTRUCTOR), method_name, params};
  auto minfo = method_signatures_.find(sig);
  if (minfo == method_signatures_.end()) {
    // Only emit error if this isn't blacklisted.
    if (!IsBlacklisted(ctx, method_name)) {
      errors->Append(MakeUndefinedMethodError(sig, pos));
    }
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

  // TODO: Check permissions.
  return minfo->second.mid;
}

Error* MethodTable::MakeUndefinedMethodError(MethodSignature sig, PosRange pos) const {
  // TODO: make this error better.

  stringstream ss;
  ss << "Couldn't find ";
  if (sig.is_constructor) {
    ss << "constructor";
  } else {
    ss << "method";
  }
  ss << " '" << sig.name << '(';
  if (sig.param_types.Size() > 0) {
    for (int i = 0; i < sig.param_types.Size(); ++i) {
      if (i > 0) {
        ss << ", ";
      }
      ss << sig.param_types.At(i).base;
    }
  }
  ss << ")'";
  return MakeSimplePosRangeError(fs_, pos, "UndefinedMethodError", ss.str());
}

Error* MethodTable::MakeInstanceMethodOnStaticError(PosRange pos) const {
  const static string msg = "Cannot call an instance method as a static method.";
  return MakeSimplePosRangeError(fs_, pos, "InstanceMethodOnStaticError", msg);
}

Error* MethodTable::MakeStaticMethodOnInstanceError(PosRange pos) const {
  const static string msg = "Cannot call a static method as an instance method.";
  return MakeSimplePosRangeError(fs_, pos, "StaticMethodOnInstanceError", msg);
}

FieldId FieldTable::ResolveAccess(TypeId callerType, CallContext ctx, string field_name, PosRange pos, ErrorList* errors) const {
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

  // TODO: Check permissions.

  return finfo->second.fid;
}

Error* FieldTable::MakeUndefinedReferenceError(string name, PosRange pos) const {
  stringstream ss;
  ss << "Undefined reference to '";
  ss << name;
  ss << '\'';
  return MakeSimplePosRangeError(fs_, pos, "UndefinedReferenceError", ss.str());
}

Error* FieldTable::MakeInstanceFieldOnStaticError(PosRange pos) const {
  const static string msg = "Cannot access an instance field without an instance.";
  return MakeSimplePosRangeError(fs_, pos, "InstanceFieldOnStaticError", msg);
}

Error* FieldTable::MakeStaticFieldOnInstanceError(PosRange pos) const {
  const static string msg = "Cannot access a static field as an instance field.";
  return MakeSimplePosRangeError(fs_, pos, "StaticFieldOnInstanceError", msg);
}

} // namespace types
