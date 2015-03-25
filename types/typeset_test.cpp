#include "types/types_test.h"

namespace types {

class TypeSetTest : public TypesTest {};

TEST_F(TypeSetTest, TwoClassesWithSameQualifiedName) {
  ParseProgram({
    {"a/Foo.java", "package foo; public class Foo {}"},
    {"b/Foo.java", "package foo; public class Foo {}"},
  });
  EXPECT_ERRS("TypeDuplicateDefinitionError: [0:26-29,1:26-29,]\n");
}

TEST_F(TypeSetTest, ClassAndPackageWithSameQualifiedName) {
  ParseProgram({
    {"a/Foo.java", "package foo.bar; public class Foo {}"},
    {"b/bar.java", "package foo; public class bar {}"},
  });
  EXPECT_ERRS("TypeDuplicateDefinitionError: [1:26-29,0:8-15,]\n");
}

TEST_F(TypeSetTest, UnknownImport) {
  ParseProgram({
    {
      "a/Foo.java",

      "import unknown.Class;\n"
      "public class Foo {\n"
      "  public Class y = null;\n"
      "}"
    },
  });
  EXPECT_ERRS("UnknownImportError(0:7-20)\n");
}

TEST_F(TypeSetTest, MultipleWildcards) {
  ParseProgram({
    {"a/bar.java", "package a; public class bar {}"},
    {"b/bar.java", "package b; public class bar {}"},
    {"c/bar.java", "package c; public class bar {}"},
    {
      "d/gee.java",

      "package d;\n"
      "import a.*;\n"
      "import b.*;\n"
      "import c.*;\n"
      "public class gee {}"
    },
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, WildcardsOverruledByPackage) {
  ParseProgram({
    {"a/bar.java", "package a; public class bar {}"},
    {"b/bar.java", "package b; public class bar {}"},
    {"c/bar.java", "package c; public class bar { public bar() {} }"},
    {
      "d/gee.java",

      "package c;\n"
      "import a.*;\n"
      "import b.*;\n"
      "public class gee extends bar {}"
    },
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, WildcardsOverruledBySingleImport) {
  ParseProgram({
    {"a/bar.java", "package a; public class bar {}"},
    {"b/bar.java", "package b; public class bar {}"},
    {"c/bar.java", "package c; public class bar { public bar() {} }"},
    {
      "d/gee.java",

      "package d;\n"
      "import a.*;\n"
      "import b.*;\n"
      "import c.bar;\n"
      "public class gee extends bar {}"
    },
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, RedundantImport) {
  ParseProgram({
    {"a/bar.java", "package a; public class bar { public bar() {} }"},
    {
      "b/gee.java",

      "package b;\n"
      "import a.*;\n"
      "import a.bar;\n"
      "public class gee extends bar {}"
    },
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, ConflictingImports) {
  ParseProgram({
    {"a/bar.java", "package a; public class bar {}"},
    {"b/bar.java", "package b; public class bar {}"},
    {
      "c/gee.java",

      "package c;\n"
      "import a.bar;\n"
      "import b.bar;\n"
      "public class gee {}"
    },
  });
  EXPECT_ERRS("TypeDuplicateDefinitionError: [2:18-23,2:32-37,]\n");
}

TEST_F(TypeSetTest, ConflictingImportAndType) {
  ParseProgram({
    {"a/bar.java", "package a; public class bar {}"},
    {"b/bar.java", "package b; import a.bar; public class bar {}"},
  });
  EXPECT_ERRS("TypeDuplicateDefinitionError: [1:18-23,1:38-41,]\n");
}

TEST_F(TypeSetTest, UnknownQualifiedName) {
  ParseProgram({
    {"bar.java", "public class bar { public unknown.pkg.Class foo; }"},
  });
  EXPECT_ERRS("UnknownTypenameError(0:26-43)\n");
}

TEST_F(TypeSetTest, QualifiedNameWithTypePrefix) {
  ParseProgram({
    {"foo/bar.java", "package foo; public class bar {}"},
    {"bar/baz.java", "package bar; public class baz {}"},
    {"test.java", "import foo.bar; public class test { public bar.baz field; }"},
  });
  EXPECT_ERRS("TypeWithTypePrefixError(2:43-50)\n");
}

TEST_F(TypeSetTest, ShortNameNoMatch) {
  ParseProgram({
    {"test.java", "public class test { public Strng field; }"},
  });
  EXPECT_ERRS("UnknownTypenameError(0:27-32)\n");
}

TEST_F(TypeSetTest, ShortNameExactMatch) {
  ParseProgram({
    {"test.java", "public class test { public String field; }"},
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, ShortNameMultipleIdenticalWildcards) {
  ParseProgram({
    {"foo/Foo.java", "package foo; public class Foo {}"},
    {
      "test.java",

      "import foo.*;\n"
      "import foo.*;\n"
      "import foo.*;\n"
      "import foo.*;\n"
      "public class test { public Foo field; }"
    },
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, ShortNameMultipleAmbiguousWildcardsNoUse) {
  ParseProgram({
    {"foo/Foo.java", "package foo; public class Foo {}"},
    {"bar/Foo.java", "package bar; public class Foo {}"},
    {
      "test.java",

      "import foo.*;\n"
      "import bar.*;\n"
      "public class test {}"
    },
  });
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, ShortNameMultipleAmbiguousWildcards) {
  ParseProgram({
    {"foo/Foo.java", "package foo; public class Foo {}"},
    {"bar/Foo.java", "package bar; public class Foo {}"},
    {
      "test.java",

      "import foo.*;\n"
      "import foo.*;\n"
      "import bar.*;\n"
      "import bar.*;\n"
      "public class test { public Foo field; }"
    },
  });
  EXPECT_ERRS("AmbiguousTypeError:[2:35-38,2:7-10,]\n");
}

TEST_F(TypeSetTest, ShortNameAmbiguousWildcardWithStdlib) {
  ParseProgram({
    {"bar/String.java", "package bar; public class String {}"},
    {
      "test.java",

      "import bar.*;\n"
      "public class test { public String field; }"
    },
  });
  EXPECT_ERRS("AmbiguousTypeError:[1:7-10,-1:-1--1,]\n");
}


TEST_F(TypeSetTest, WildcardOfNonExistentPackage) {
  ParseProgram({
    {"test.java", "import non.existent.pkg.*;"},
  });
  EXPECT_ERRS("UnknownPackageError(0:7-23)\n");
}

} // namespace types
