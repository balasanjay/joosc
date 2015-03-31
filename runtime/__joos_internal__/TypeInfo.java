R"(
package __joos_internal__;

public class TypeInfo {
  protected int tid = 0;
  protected TypeInfo[] parents = null;

  // 0 -> Unset
  // 1 -> Yes
  // -1 -> No
  // TODO: Make this byte[].
  protected int[] ancestor_map = null;

  // When false, this class hasn't been initialized yet.
  // This will occur while setting up all of the TypeInfos.
  // Allow all instanceof checks to pass during this initialization.
  protected static boolean initialized = true;
  public static int num_types;

  public TypeInfo(int tid, TypeInfo[] parents) {
    this.tid = tid;
    this.parents = parents;
  }

  public static boolean InstanceOf(TypeInfo child, TypeInfo ancestor) {
    if (!TypeInfo.initialized) {
      return true;
    }
    if (child == ancestor) {
      return true;
    }

    if (child.ancestor_map == null) {
      child.ancestor_map = new int[TypeInfo.num_types];
      // Initialized to 0 -> Unset.
    }

    int lookup = child.ancestor_map[ancestor.tid];
    // Already checked this - return cached value.
    if (lookup != 0) {
      return lookup == 1;
    }

    for (int i = 0; lookup == 0 && i < child.parents.length; i = i + 1) {
      // If parent is ancestor, return true immediately.
      TypeInfo parent = child.parents[i];
      if (parent == ancestor || TypeInfo.InstanceOf(parent, ancestor)) {
        lookup = 1; // True.
      }
    }
    if (lookup == 0) {
      lookup = -1;
    }

    child.ancestor_map[ancestor.tid] = lookup;
    return lookup == 1;
  }
}
)"
