R"(
package __joos_internal__;

public class StringOps {
  public static String nullStr() {
    return new String("null");
  }

  public static String Str(Object o) {
    if (o == null) {
      return StringOps.nullStr();
    }

    String s = o.toString();
    if (s == null) {
      return StringOps.nullStr();
    }

    return s;
  }
}
)"
