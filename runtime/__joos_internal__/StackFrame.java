R"(
package __joos_internal__;

public final class StackFrame {
  // Group pointers first, then ints.
  protected String file;
  protected String type;
  protected String method;
  protected int line;

  public final void Print() {
    print(file);
    print(":");
    print(line);
    print(" ");
    print(type);
    print(".");
    println(method);
  }
}
)"
