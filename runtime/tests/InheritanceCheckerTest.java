package runtime.tests;

import runtime.InheritanceChecker;
import runtime.TypeInfo;

public class InheritanceCheckerTest {
  protected int tests = 0;
  protected int failures = 0;

  protected InheritanceCheckerTest() {}

  protected void ExpectEq(boolean expected, boolean actual) {
    tests = tests + 1;
    if (expected == actual) {
      System.out.print(".");
    } else {
      failures = failures + 1;
      System.out.print("E");
    }
  }

  protected void End() {
    System.out.println("");
    System.out.println("Ran " + tests + " tests. Got " + failures + " failures.");
  }

  protected void test1() {
    /*
     *      3
     *      |
     *    4 2
     *     \|
     *      1
     *      |
     *      0
     */
    int num_types = 5;
    TypeInfo[] type_infos = new TypeInfo[num_types];
    // 0.
    {
      int[] parents = new int[1];
      parents[0] = 1;
      type_infos[0] = new TypeInfo(num_types, parents);
    }
    // 1.
    {
      int[] parents = new int[2];
      parents[0] = 2;
      parents[1] = 4;
      type_infos[1] = new TypeInfo(num_types, parents);
    }
    // 2.
    {
      int[] parents = new int[1];
      parents[0] = 3;
      type_infos[2] = new TypeInfo(num_types, parents);
    }
    // 3.
    {
      int[] parents = new int[0];
      type_infos[3] = new TypeInfo(num_types, parents);
    }
    // 4.
    {
      int[] parents = new int[0];
      type_infos[4] = new TypeInfo(num_types, parents);
    }

    InheritanceChecker.Initialize(type_infos);
    ExpectEq(true,  InheritanceChecker.IsAncestor(0, 1));
    ExpectEq(true,  InheritanceChecker.IsAncestor(0, 3));
    ExpectEq(true,  InheritanceChecker.IsAncestor(0, 4));
    ExpectEq(true,  InheritanceChecker.IsAncestor(1, 4));
    ExpectEq(false, InheritanceChecker.IsAncestor(2, 4));
    ExpectEq(false, InheritanceChecker.IsAncestor(0, 0));
    ExpectEq(false, InheritanceChecker.IsAncestor(2, 0));
    ExpectEq(false, InheritanceChecker.IsAncestor(4, 3));
  }

  public static void main(String[] args) {
    InheritanceCheckerTest test = new InheritanceCheckerTest();
    test.test1();
    test.End();
  }
}
