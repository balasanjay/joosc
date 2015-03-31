R"(
package __joos_internal__;

public class TypeInfo {
  // 0 -> Unset
  // 1 -> Yes
  // -1 -> No
  protected int tid = 0;
  protected TypeInfo[] parents = null;
  protected int[] ancestor_map = null;

  // When false, this class hasn't been initialized yet.
  // This will occur while setting up all of the TypeInfos.
  // Allow all instanceof checks to pass during this initialization.
  protected static boolean initialized = true;
  protected static int num_types;

  public TypeInfo(int tid, TypeInfo[] parents) {
    this.tid = tid;
    this.parents = parents;
  }

  public boolean InstanceOf(TypeInfo ancestor) {
    if (!TypeInfo.initialized) {
      return true;
    }
    if (this == ancestor) {
      return true;
    }

    if (ancestor_map == null) {
      ancestor_map = new int[TypeInfo.num_types];
      // Initialized to 0 -> Unset.
    }

    int lookup = ancestor_map[ancestor.tid];
    // Already checked this - return cached value.
    if (lookup != 0) {
      return lookup == 1;
    }

    for (int i = 0; lookup == 0 && i < parents.length; i = i + 1) {
      // If parent is ancestor, return true immediately.
      TypeInfo parent = parents[i];
      if (parent == ancestor || parent.InstanceOf(ancestor)) {
        lookup = 1; // True.
      }
    }
    if (lookup == 0) {
      lookup = -1;
    }

    ancestor_map[ancestor.tid] = lookup;
    return lookup == 1;
  }
}
)"
