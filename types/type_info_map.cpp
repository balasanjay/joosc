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

} // namespace

TypeInfoMap TypeInfoMap::kEmptyTypeInfoMap = TypeInfoMap({});
MethodTable MethodTable::kEmptyMethodTable = MethodTable({}, {}, false);
MethodTable MethodTable::kErrorMethodTable = MethodTable();
MethodInfo MethodTable::kErrorMethodInfo = MethodInfo{kErrorMethodId, TypeId::kError, {}, TypeId::kError, PosRange(-1, -1, -1), {false, "", TypeIdList({})}};

Error* TypeInfoMapBuilder::MakeConstructorNameError(PosRange pos) const {
  return MakeSimplePosRangeError(fs_, pos, "ConstructorNameError", "Constructors must have the same name as its class.");
}

Error* TypeInfoMapBuilder::MakeExtendsCycleError(const vector<TypeInfo>& cycle) const {
  const FileSet* fs = fs_;
  return MakeError([=](ostream* out, const OutputOptions& opt) {
    assert(cycle.size() > 1);
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
      PrintDiagnosticHeader(out, opt, fs, cur.pos, DiagnosticClass::INFO, msg.str());
      PrintRangePtr(out, opt, fs, cur.pos);
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

MethodTable TypeInfoMapBuilder::MakeResolvedMethodTable(TypeInfo* tinfo, const MethodTable::MethodSignatureMap& good_methods, const set<string>& bad_methods, bool has_bad_constructor, const map<ast::TypeId, TypeInfo>& sofar, ErrorList* out) {
  MethodTable::MethodSignatureMap new_good_methods(good_methods);
  set<string> new_bad_methods(bad_methods);

  TypeIdList parents = Concat({tinfo->extends, tinfo->implements});

  for (int i = 0; i < parents.Size(); ++i) {
    const auto info_pair = sofar.find(parents.At(i));
    assert(info_pair != sofar.cend());
    const TypeInfo& pinfo = info_pair->second;

    // Early return if any of our parents are broken.
    if (pinfo.methods.all_blacklisted_) {
      return MethodTable::kErrorMethodTable;
    }

    // We cannot inherit from a parent that is declared final.
    if (pinfo.mods.HasModifier(FINAL)) {
      // TODO: Emit error. (parent final)
      out->Append(MakeSimplePosRangeError(fs_, pinfo.mods.GetModifierToken(FINAL).pos, "ParentFinalError", "ParentFinalError"));
      continue;
    }

    for (const auto& psig_pair : pinfo.methods.method_signatures_) {
      const MethodSignature& psig = psig_pair.first;
      const MethodInfo& implicit_pminfo = psig_pair.second;
      // Make all implicit modifiers explicit.
      const MethodInfo pminfo = FixMods(pinfo, implicit_pminfo);

      // Skip constructors since they are not inherited.
      if (psig.is_constructor) {
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

      const MethodInfo& implicit_mminfo = msig_pair->second;
      // Make all implicit modifiers explicit.
      const MethodInfo mminfo = FixMods(*tinfo, implicit_mminfo);

      // We cannot inherit methods of the same signature but differing return
      // types.
      if (pminfo.return_type != mminfo.return_type) {
        // TODO: Emit error (different return types).
        out->Append(MakeSimplePosRangeError(fs_, mminfo.pos, "DifferingReturnTypeError", "DifferingReturnTypeError"));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }

      // Inheriting methods that are static or with a static overload are not
      // allowed.
      if (pminfo.mods.HasModifier(lexer::STATIC) || mminfo.mods.HasModifier(lexer::STATIC)) {
        // TODO: Emit error (static method clash).
        out->Append(MakeSimplePosRangeError(fs_, mminfo.pos, "StaticMethodOverrideError", "StaticMethodOverrideError"));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }

      assert(!pminfo.mods.HasModifier(NATIVE));
      assert(!mminfo.mods.HasModifier(NATIVE));

      // We can't lower visibility of inherited methods.
      if (pminfo.mods.HasModifier(PUBLIC) && mminfo.mods.HasModifier(PROTECTED)) {
        // TODO: Emit error (lower visibility).
        out->Append(MakeSimplePosRangeError(fs_, mminfo.pos, "LowerVisibilityError", "LowerVisibilityError"));
        new_bad_methods.insert(mminfo.signature.name);
        continue;
      }
      bool is_protected = mminfo.mods.HasModifier(PROTECTED);

      // We can't override final methods.
      if (pminfo.mods.HasModifier(FINAL)) {
        // TODO: Emit error (can't override final).
        out->Append(MakeSimplePosRangeError(fs_, mminfo.pos, "OverrideFinalMethodError", "OverrideFinalMethodError"));
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

    // Union sets of disallowed names from the parent.
    new_bad_methods.insert(pinfo.methods.bad_methods_.begin(), pinfo.methods.bad_methods_.end());
  }

  // If we have abstract methods, we must also be abstract.
  {
    using CMSMPair = pair<MethodSignature, MethodInfo>;
    auto is_abstract_method = [](CMSMPair pair) {
      return pair.second.mods.HasModifier(ABSTRACT);
    };
    // TODO: first_of
    bool has_abstract = std::any_of(new_good_methods.cbegin(), new_good_methods.cend(), is_abstract_method);

    if (has_abstract && !tinfo->mods.HasModifier(ABSTRACT)) {
      // TODO: Emit error.
      out->Append(MakeSimplePosRangeError(fs_, tinfo->pos, "NeedAbstractClassError", "NeedAbstractClassError"));
    }
  }

  return MethodTable(new_good_methods, new_bad_methods, has_bad_constructor);
}

// Builds valid MethodTables for a TypeInfo. Emits errors if methods for the
// type are invalid.
void TypeInfoMapBuilder::BuildMethodTable(MInfoIter begin, MInfoIter end, TypeInfo* tinfo, MethodId* cur_mid, const map<TypeId, TypeInfo>& sofar, ErrorList* out) {
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
      assert(ndups > 1);

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
      out->Append(MakeDuplicateDefinitionError(fs_, defs, msgstream.str(), lbegin->signature.name));
      bad_methods.insert(lbegin->signature.name);
    };

    FindEqualRanges(begin, end, eq_cmp, cb);
  }

  tinfo->methods = MakeResolvedMethodTable(tinfo, good_methods, bad_methods, has_bad_constructor, sofar, out);
}

TypeInfoMap TypeInfoMapBuilder::Build(base::ErrorList* out) {
  map<TypeId, TypeInfo> typeinfo;
  set<TypeId> bad_types;

  for (const auto& entry : type_entries_) {
    typeinfo.insert({entry.type, entry});
  }

  ValidateExtendsImplementsGraph(&typeinfo, &bad_types, out);

  // Sort MethodInfo vector by the topological ordering of the types.
  {
    auto cmp = [&typeinfo](const MethodInfo& lhs, const MethodInfo& rhs) {
      return typeinfo.at(lhs.class_type).top_sort_index < typeinfo.at(rhs.class_type).top_sort_index;
    };
    stable_sort(method_entries_.begin(), method_entries_.end(), cmp);
  }

  // Populate MethodTables for each TypeInfo.
  {
    // TODO: Take &bad_types as an argument, and immediately skip types that are bad.
    // TODO: Catch classes with no methods.
    MethodId cur_mid = kFirstMethodId;

    auto cmp = [](const MethodInfo& lhs, const MethodInfo& rhs) {
      return lhs.class_type == rhs.class_type;
    };

    auto cb = [&](MInfoIter begin, MInfoIter end, i64) {
      TypeInfo* tinfo = &typeinfo.at(begin->class_type);
      BuildMethodTable(begin, end, tinfo, &cur_mid, typeinfo, out);
    };

    FindEqualRanges(method_entries_.begin(), method_entries_.end(), cmp, cb);
  }

  // TODO: take bad_types in the constructor. Any lookups of TypeIds from
  // bad_types should yield kErrorTypeInfo, or something like it.
  return TypeInfoMap(typeinfo);
}

void TypeInfoMapBuilder::ValidateExtendsImplementsGraph(map<TypeId, TypeInfo>* types, set<TypeId>* bad, ErrorList* errors) {
  using IdInfoMap = map<TypeId, TypeInfo>;

  // Bind a reference to make the code more readable.
  IdInfoMap& all_types = *types;
  set<TypeId>& bad_types = *bad;

  // Ensure that we blacklist any classes that introduce invalid edges into the
  // graph.
  PruneInvalidGraphEdges(all_types, &bad_types, errors);

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
      all_types.at(topsort.at(i)).top_sort_index = i;
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
      assert(typeinfo.implements.Size() == 0);

      // An interface can only extend other interfaces.
      for (int i = 0; i < typeinfo.extends.Size(); ++i) {
        TypeId extends_tid = typeinfo.extends.At(i);
        if (!match_relationship(extends_tid, type, TypeKind::INTERFACE)) {
          errors->Append(MakeInterfaceExtendsClassError(fs_, typeinfo.pos, all_types.at(extends_tid).name));
          bad_types->insert(type);
        }
      }
      continue;
    }

    assert(typeinfo.kind == TypeKind::CLASS);
    assert(typeinfo.extends.Size() <= 1);
    if (typeinfo.extends.Size() == 1) {
      TypeId parent_tid = typeinfo.extends.At(0);
      if (!match_relationship(parent_tid, type, TypeKind::CLASS)) {
        errors->Append(MakeClassExtendsInterfaceError(fs_, typeinfo.pos, all_types.at(parent_tid).name));
        bad_types->insert(type);
      }
    }
    for (int i = 0; i < typeinfo.implements.Size(); ++i) {
      TypeId implement_tid = typeinfo.implements.At(i);
      if (!match_relationship(implement_tid, type, TypeKind::INTERFACE)) {
        errors->Append(MakeClassImplementsClassError(fs_, typeinfo.pos, all_types.at(implement_tid).name));
        bad_types->insert(type);
      }
    }
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
      assert(iter != path.end());

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
      assert(path.size() == prev_path_size);

      if (!ok) {
        break;
      }
    }

    assert(path.size() > 0);
    assert(path.back() == tid);
    path.pop_back();
    open.erase(tid);

    if (!ok) {
      bad.insert(tid);
      return false;
    }

    assert(ok);
    good.insert(tid);
    sorted.push_back(tid);

    return true;
  };

  for (const auto& type : edges) {
    fn(type.first);
  }

  return sorted;
}

} // namespace types
