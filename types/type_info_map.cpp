#include "types/type_info_map.h"

#include "ast/ast.h"
#include "base/algorithm.h"
#include "lexer/lexer.h"
#include "types/types_internal.h"

using std::initializer_list;
using std::tie;

using ast::ModifierList;
using ast::TypeId;
using ast::TypeKind;
using base::Error;
using base::ErrorList;
using base::FileSet;
using base::FindEqualRanges;
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

} // namespace

TypeInfoMap TypeInfoMap::kEmptyTypeInfoMap = TypeInfoMap({});
MethodTable MethodTable::kEmptyMethodTable = MethodTable({}, {}, false);
MethodTable MethodTable::kErrorMethodTable = MethodTable();
MethodInfo MethodTable::kErrorMethodInfo = MethodInfo{kErrorMethodId, TypeId::kError, {}, TypeId::kError, PosRange(-1, -1, -1), {false, "", TypeIdList({})}};

Error* TypeInfoMapBuilder::MakeConstructorNameError(PosRange pos) const {
  return MakeSimplePosRangeError(fs_, pos, "ConstructorNameError", "Constructors must have the same name as its class.");
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

      // Inheriting methods that are static or overriding with a static method
      // are not allowed.
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
    auto is_abstract_method = [](pair<MethodSignature, MethodInfo> pair) {
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
  for (const auto& entry : type_entries_) {
    typeinfo.insert({entry.type, entry});
  }

  // TODO(sjy): assign top_sort_index.

  // Sort MethodInfo vector by the topological ordering of the types.
  {
    auto cmp = [&typeinfo](const MethodInfo& lhs, const MethodInfo& rhs) {
      return typeinfo.at(lhs.class_type).top_sort_index < typeinfo.at(rhs.class_type).top_sort_index;
    };
    stable_sort(method_entries_.begin(), method_entries_.end(), cmp);
  }

  // Populate MethodTables for each TypeInfo.
  {
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

  return TypeInfoMap(typeinfo);
}

} // namespace types
