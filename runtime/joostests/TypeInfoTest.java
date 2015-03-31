package joostests;

import __joos_internal__.TypeInfo;

public class TypeInfoTest {
  protected int tests = 0;
  protected int failures = 0;

  protected TypeInfoTest() {}

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
    TypeInfo[] types = new TypeInfo[num_types];
    // 3.
    {
      TypeInfo[] parents = new TypeInfo[0];
      types[3] = new TypeInfo(3, parents);
    }
    // 4.
    {
      TypeInfo[] parents = new TypeInfo[0];
      types[4] = new TypeInfo(4, parents);
    }
    // 2.
    {
      TypeInfo[] parents = new TypeInfo[1];
      parents[0] = types[3];
      types[2] = new TypeInfo(2, parents);
    }
    // 1.
    {
      TypeInfo[] parents = new TypeInfo[2];
      parents[0] = types[2];
      parents[1] = types[4];
      types[1] = new TypeInfo(1, parents);
    }
    // 0.
    {
      TypeInfo[] parents = new TypeInfo[1];
      parents[0] = types[1];
      types[0] = new TypeInfo(0, parents);
    }

    ExpectEq(true,  types[0].InstanceOf(types[1]));
    ExpectEq(true,  types[0].InstanceOf(types[3]));
    ExpectEq(true,  types[0].InstanceOf(types[4]));
    ExpectEq(true,  types[1].InstanceOf(types[4]));
    ExpectEq(true,  types[0].InstanceOf(types[0]));
    ExpectEq(false, types[2].InstanceOf(types[4]));
    ExpectEq(false, types[2].InstanceOf(types[0]));
    ExpectEq(false, types[4].InstanceOf(types[3]));
  }

  public static void main(String[] args) {
    TypeInfoTest test = new TypeInfoTest();
    test.test1();
    test.End();
  }
}
