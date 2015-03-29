package runtime;

public class InheritanceChecker {
  protected TypeInfo[] type_infos;

  public InheritanceChecker(TypeInfo[] type_infos) {
    this.type_infos = type_infos;
  }

  public boolean IsAncestor(int child_type, int ancestor_type) {
    TypeInfo child = type_infos[child_type];

    // Check if already memoized.
    int ancestor_lookup = child.IsAncestor(ancestor_type);
    if (ancestor_lookup == TypeInfo.YES) {
      return true;
    } else if (ancestor_lookup == TypeInfo.NO) {
      return false;
    }

    // Unknown - need to search.
    boolean is_ancestor = IsAncestorRec(child_type, ancestor_type);
    // Record value for subsequent lookups.
    child.SetAncestor(ancestor_type, is_ancestor);
    return is_ancestor;
  }

  protected boolean IsAncestorRec(int child_type, int ancestor_type) {
    TypeInfo child = type_infos[child_type];
    int[] parents = child.Parents();
    for (int i = 0; i < parents.length; i = i + 1) {
      // If parent is ancestor, return true immediately.
      if (parents[i] == ancestor_type) {
        return true;
      }

      // Recurse using memoized lookup on parents.
      // This call will also record the intermediate ancestory relation between the parent and the ancestor.
      if (IsAncestor(parents[i], ancestor_type) ){
        return true;
      }
    }

    return false;
  }


}
