#include "types/type_info_map.h"

#include "base/algorithm.h"
#include "types/types_internal.h"

using std::function;
using std::ostream;
using std::tie;

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

namespace types {

TypeInfoMap TypeInfoMap::kEmptyTypeInfoMap = TypeInfoMap({});
MethodTable MethodTable::kEmptyMethodTable = MethodTable({}, {}, false);
MethodTable MethodTable::kErrorMethodTable = MethodTable();

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

// Builds valid MethodTables for a TypeInfo. Emits errors if methods for the
// type are invalid.
void TypeInfoMapBuilder::BuildMethodTable(MInfoIter begin, MInfoIter end, TypeInfo* tinfo, MethodId* cur_mid, const map<TypeId, TypeInfo>& sofar, ErrorList* out) {
  // Sort all MethodInfo to cluster them by signature.
  auto lt_cmp = [](const MethodInfo& lhs, const MethodInfo& rhs) {
    return tie(lhs.is_constructor, lhs.signature) < tie(rhs.is_constructor, rhs.signature);
  };
  stable_sort(begin, end, lt_cmp);

  vector<MethodTableParam> good_methods;
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
        if (cur->is_constructor && cur->signature.name != tinfo->name) {
          out->Append(MakeConstructorNameError(cur->pos));
          has_bad_constructor = true;
        }
      }

      // Add non-duplicate MethodInfo to the MethodTable.
      if (ndups == 1) {
        good_methods.push_back({*lbegin, *cur_mid});
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
      if (lbegin->is_constructor) {
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

  // TODO: update MethodTable with methods from parents.
  tinfo->methods = MethodTable(good_methods, bad_methods, has_bad_constructor);
}

TypeInfoMap TypeInfoMapBuilder::Build(base::ErrorList* out) {
  map<TypeId, TypeInfo> typeinfo;
  for (const auto& entry : type_entries_) {
    typeinfo.insert({entry.type, entry});
  }

  ValidateExtendsImplementsGraph(&typeinfo, out);

  // Sort MethodInfo vector by the topological ordering of the types.
  {
    auto cmp = [&typeinfo](const MethodInfo& lhs, const MethodInfo& rhs) {
      return typeinfo.at(lhs.class_type).top_sort_index < typeinfo.at(rhs.class_type).top_sort_index;
    };
    stable_sort(method_entries_.begin(), method_entries_.end(), cmp);
  }

  // Populate MethodTables for each TypeInfo.
  {
    MethodId cur_mid = 1;

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

void TypeInfoMapBuilder::ValidateExtendsImplementsGraph(map<TypeId, TypeInfo>* m, ErrorList* errors) {
  using IdInfoMap = map<TypeId, TypeInfo>;

  // Bind a reference to make the code more readable.
  IdInfoMap& all_types = *m;
  set<TypeId> blacklisted_types;

  // Ensure that we blacklist any classes that introduce invalid any edges into
  // the graph.
  PruneInvalidGraphEdges(all_types, &blacklisted_types, errors);

  // Now build a combined graph of edges.
  multimap<TypeId, TypeId> edges;
  for (const auto& type_pair : all_types) {
    const TypeId type = type_pair.first;
    const TypeInfo& typeinfo = type_pair.second;

    if (blacklisted_types.count(type) == 1) {
      continue;
    }

    for (int i = 0; i < typeinfo.extends.Size(); ++i) {
      edges.insert({type, typeinfo.extends.At(i)});
    }
    for (int i = 0; i < typeinfo.implements.Size(); ++i) {
      edges.insert({type, typeinfo.implements.At(i)});
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
    VerifyAcyclicGraph(edges, &blacklisted_types, cycle_cb);
  }

  // TODO: figure out what jon needs here and make it thusly.
}

void TypeInfoMapBuilder::PruneInvalidGraphEdges(const map<TypeId, TypeInfo>& all_types, set<TypeId>* blacklisted_types, ErrorList* errors) {
  // Blacklists child if parent doesn't exist in all_types, or parent's kind
  // doesn't match expected_parent_kind.
  auto match_relationship = [&](TypeId parent, TypeId child, TypeKind expected_parent_kind) {
    auto parent_iter = all_types.find(parent);
    if (parent_iter == all_types.end()) {
      blacklisted_types->insert(child);
      return false;
    }

    if (parent_iter->second.kind != expected_parent_kind) {
      blacklisted_types->insert(child);
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
          // TODO: emit an error.
          throw;
        }
      }
      continue;
    }

    assert(typeinfo.kind == TypeKind::CLASS);
    assert(typeinfo.extends.Size() <= 1);
    if (typeinfo.extends.Size() == 1) {
      TypeId parent_tid = typeinfo.extends.At(0);
      if (!match_relationship(parent_tid, type, TypeKind::CLASS)) {
        // TODO: emit an error.
        throw;
      }
    }
    for (int i = 0; i < typeinfo.implements.Size(); ++i) {
      TypeId implement_tid = typeinfo.implements.At(i);
      if (!match_relationship(implement_tid, type, TypeKind::INTERFACE)) {
        // TODO: emit an error.
        throw;
      }
    }
  }
}

void TypeInfoMapBuilder::VerifyAcyclicGraph(const multimap<TypeId, TypeId>& edges, set<TypeId>* blacklisted_types, function<void(const vector<TypeId>& cycle)> cb) {
  // Both of these store different representations of the current recursion path.
  set<TypeId> open;
  vector<TypeId> path;

  // Tracks known values of TypeIds.
  set<TypeId> good;
  set<TypeId>& bad = *blacklisted_types;

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
}



} // namespace types
