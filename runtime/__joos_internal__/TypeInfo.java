package __joos_internal__;

public class TypeInfo {
  public static int UNSET = 0;
  public static int YES = 1;
  public static int NO = -1;

  protected int[] ancestor_map;
  protected int[] parents;

  public TypeInfo(int num_types, int[] parents) {
    ancestor_map = new int[num_types];
    this.parents = parents;
    for (int i = 0; i < num_types; i = i + 1) {
      ancestor_map[i] = TypeInfo.UNSET;
    }
  }

  public int[] Parents() {
    return parents;
  }

  public int IsAncestor(int type) {
    return ancestor_map[type];
  }

  public void SetAncestor(int type, boolean is_ancestor) {
    if (is_ancestor) {
      ancestor_map[type] = TypeInfo.YES;
    } else {
      ancestor_map[type] = TypeInfo.NO;
    }
  }
}
