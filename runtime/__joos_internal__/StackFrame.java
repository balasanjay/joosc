R"(
package __joos_internal__;

public final class StackFrame {
  public static boolean ENABLE_EXCEPTIONS = false;

  // Group pointers first, then ints.
  protected String file;
  protected String type;
  protected String method;
  protected int line;

  public static void PrintException(int type) {
    if (!StackFrame.ENABLE_EXCEPTIONS) {
      return;
    }

    System.out.println();
    if (type == 0) {
      System.out.println("java.lang.ArithmeticException");
    } else if (type == 1) {
      System.out.println("java.lang.NullPointerException");
    } else if (type == 2) {
      System.out.println("java.lang.ArrayIndexOutOfBoundsException");
    } else if (type == 3) {
      System.out.println("java.lang.NegativeArraySizeException");
    } else if (type == 4) {
      System.out.println("java.lang.ClassCastException");
    } else if (type == 5) {
      System.out.println("java.lang.ArrayStoreException");
    }
  }

  public final void Print() {
    if (!StackFrame.ENABLE_EXCEPTIONS) {
      return;
    }

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
