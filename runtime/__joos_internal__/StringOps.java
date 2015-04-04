R"(
package __joos_internal__;

public class StringOps {
  public static String Str(Object o) {
    if (o == null) {
      return "null";
    }

    String s = o.toString();
    if (s == null) {
      return "null";
    }

    return s;
  }
}
)"
