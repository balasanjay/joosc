
#include "runtime/runtime.h"

namespace runtime {

const string TypeInfoFile = R"(
package __joos_internal__;

public class TypeInfo {
  // 0 -> Unset
  // 1 -> Yes
  // -1 -> No
  protected int[] ancestor_map;

  // Type IDs of parents.
  protected int[] parents;

  // All TypeInfos.
  public static TypeInfo[] types;

  // When false, this class hasn't been initialized yet.
  // This will occur while setting up all of the TypeInfos.
  // Allow all instanceof checks to pass during this initialization.
  public static boolean initialized = true;

  public TypeInfo(int[] parents) {
    this.parents = parents;
  }

  public boolean InstanceOf(int ancestor_id) {
    if (!TypeInfo.initialized) {
      return true;
    }
    if (ancestor_map == null) {
      // TODO: Fix size based off of first ref type base, and maybe use the offset.
      ancestor_map = new int[TypeInfo.types.length + 25];
      // Initialized to 0 -> Unset.
    }

    int lookup = ancestor_map[ancestor_id];
    // Already checked this - return cached value.
    if (lookup != 0) {
      return lookup == 1;
    }

    for (int i = 0; lookup == 0 && i < parents.length; i = i + 1) {
      // If parent is ancestor, return true immediately.
      int parent_id = parents[i];
      if (parent_id == ancestor_id) {
        lookup = 1; // True.
      } else {
        if (TypeInfo.types[parent_id].InstanceOf(ancestor_id)) {
          lookup = 1;
        }
      }
    }
    if (lookup == 0) {
      lookup = -1;
    }

    ancestor_map[ancestor_id] = lookup;
    return lookup == 1;
  }
}
)";

} // namespace runtime

