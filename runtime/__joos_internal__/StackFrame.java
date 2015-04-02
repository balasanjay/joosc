R"(
package __joos_internal__;

public final class StackFrame {
  // Group pointers first, then ints.
  protected String file;
  protected String type;
  protected String method;
  protected int line;

  public static void PrintException(int type) {
    System.out.println();
    if (type == 0) {
      System.out.println("java.lang.ArithmeticException");
    } else if (type == 1) {
      System.out.println("java.lang.NullPointerException");
    }
  }

  public final void Print() {
    System.out.print(file);
    System.out.print(":");
    System.out.print(line);
    System.out.print(" ");
    System.out.print(type);
    System.out.print(".");
    System.out.println(method);
  }
}
)"
