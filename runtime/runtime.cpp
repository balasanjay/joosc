#include "runtime/runtime.h"

#include "base/file.h"

namespace runtime {

namespace {

  string GenTypeInfoHolder(const types::TypeInfoMap& tinfo_map) {
    const map<ast::TypeId, types::TypeInfo>& type_map = tinfo_map.GetTypeMap();
    int num_types = type_map.size();

    stringstream ss;
    ss << "package __joos_internal__;"
       << "public class InstanceOfGen {"
       << "  public static bool InstanceOf(int lhs_type, rhs_type) {"
       << "    return lhs_type == rhs_type || InheritanceChecker.IsAncestor(rhs_type, lhs_type);"
       << "  }"
       << "  public static void Initialize() {"
       << "    TypeInfo[] type_infos = new TypeInfo[" << num_types << "];";

    for (auto entry : type_map) {
      ast::TypeId tid = entry.first;
      const types::TypeInfo& tinfo = entry.second;
      int num_parents = tinfo.extends.Size() + tinfo.implements.Size();
      ss << "  {";
      ss << "    int[] parents = new int[" << num_parents << "];";
      int par_idx = 0;
      for (int i = 0; i < tinfo.extends.Size(); ++i) {
        ss << "    parents[" << par_idx << "] = " << tinfo.extends.At(i).base << ";";
        ++par_idx;
      }
      for (int i = 0; i < tinfo.extends.Size(); ++i) {
        ss << "    parents[" << par_idx << "] = " << tinfo.extends.At(i).base << ";";
        ++par_idx;
      }
      ss << "type_infos[" << tid.base << "] = new TypeInfo(" << num_types << ", parents)";
      ss << "  }";
    }

    ss  << "   InheritanceChecker.Initialize(type_infos);"
       << "  }"
       << "}";
    return ss.str();
  }

} // namespace

base::FileSet* GenerateRuntimeFiles(const types::TypeInfoMap& tinfo_map) {
  base::FileSet::Builder builder;
  builder.AddDiskFile("runtime/__joos_internal__/TypeInfo.java");
  builder.AddDiskFile("runtime/__joos_internal__/InheritanceChecker.java");
  builder.AddStringFile("runtime/__joos_internal__/InstanceOfGen.java", GenTypeInfoHolder(tinfo_map));

  base::FileSet* fs = nullptr;
  base::ErrorList errors;
  builder.Build(&fs, &errors);
  CHECK(!errors.IsFatal());

  return fs;
}

} // namespace runtime

