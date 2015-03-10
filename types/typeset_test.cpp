#include "third_party/gtest/gtest.h"
#include "types/types_test.h"

using ast::Program;

#define EXPECT_ERRS(msg) EXPECT_EQ(msg, testing::PrintToString(errors_))
#define EXPECT_NO_ERRS() EXPECT_EQ(0, errors_.Size())
#define PRINT_ERRS() errors_.PrintTo(&std::cerr, base::OutputOptions::kUserOutput, fs_.get())

namespace types {

class TypeSetTest : public ::testing::Test {
protected:
  sptr<const Program> ParseProgram(const vector<pair<string, string>>& file_contents) {
    base::FileSet* fs;
    sptr<const Program> program = ParseProgramWithStdlib(&fs, file_contents, &errors_);
    fs_.reset(fs);
    return program;
  }

  base::ErrorList errors_;
  uptr<base::FileSet> fs_;
};

TEST_F(TypeSetTest, TwoClassesWithSameQualifiedName) {
  vector<pair<string, string>> test_files = {
    {"a/Foo.java", "package foo; public class Foo {}"},
    {"b/Foo.java", "package foo; public class Foo {}"},
  };
  ParseProgram(test_files);
  EXPECT_ERRS("TypeDuplicateDefinitionError: [0:26-29,1:26-29,]\n");
}

TEST_F(TypeSetTest, ClassAndPackageWithSameQualifiedName) {
  vector<pair<string, string>> test_files = {
    {"a/Foo.java", "package foo.bar; public class Foo {}"},
    {"b/bar.java", "package foo; public class bar {}"},
  };
  ParseProgram(test_files);
  EXPECT_ERRS("TypeDuplicateDefinitionError: [1:26-29,0:12-15,]\n");
}

TEST_F(TypeSetTest, UnknownImport) {
  vector<pair<string, string>> test_files = {
    {
      "a/Foo.java",

      "import unknown.Class;\n"
      "public class Foo {\n"
      "  public Class y = null;\n"
      "}"
    },
  };
  ParseProgram(test_files);
  EXPECT_ERRS("UnknownImportError(0:7-20)\n");
}

TEST_F(TypeSetTest, MultipleWildcards) {
  vector<pair<string, string>> test_files = {
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
  };
  ParseProgram(test_files);
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, MultipleWildcardsAmbiguity) {
  vector<pair<string, string>> test_files = {
    {"a/bar.java", "package a; public class bar {}"},
    {"b/bar.java", "package b; public class bar {}"},
    {"c/bar.java", "package c; public class bar {}"},
    {
      "d/gee.java",

      "package d;\n"
      "import a.*;\n"
      "import b.*;\n"
      "import c.*;\n"
      "public class gee extends bar {}"
    },
  };
  ParseProgram(test_files);
  EXPECT_ERRS("AmbiguousType(3:72-75)\n");
}

TEST_F(TypeSetTest, WildcardsOverruledByPackage) {
  vector<pair<string, string>> test_files = {
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
  };
  ParseProgram(test_files);
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, WildcardsOverruledBySingleImport) {
  vector<pair<string, string>> test_files = {
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
  };
  ParseProgram(test_files);
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, RedundantImport) {
  vector<pair<string, string>> test_files = {
    {"a/bar.java", "package a; public class bar { public bar() {} }"},
    {
      "b/gee.java",

      "package b;\n"
      "import a.*;\n"
      "import a.bar;\n"
      "public class gee extends bar {}"
    },
  };
  ParseProgram(test_files);
  EXPECT_NO_ERRS();
}

TEST_F(TypeSetTest, ConflictingImports) {
  vector<pair<string, string>> test_files = {
    {"a/bar.java", "package a; public class bar {}"},
    {"b/bar.java", "package b; public class bar {}"},
    {
      "c/gee.java",

      "package c;\n"
      "import a.bar;\n"
      "import b.bar;\n"
      "public class gee {}"
    },
  };
  ParseProgram(test_files);
  EXPECT_ERRS("DuplicateCompUnitNames: [2:32-37,2:18-23,]\n");
}

TEST_F(TypeSetTest, ConflictingImportAndType) {
  vector<pair<string, string>> test_files = {
    {"a/bar.java", "package a; public class bar {}"},
    {"b/bar.java", "package b; import a.bar; public class bar {}"},
  };
  ParseProgram(test_files);
  EXPECT_ERRS("DuplicateCompUnitNames: [1:38-41,1:18-23,]\n");
}

TEST_F(TypeSetTest, UnknownQualifiedName) {
  vector<pair<string, string>> test_files = {
    {"bar.java", "public class bar { public unknown.pkg.Class foo = null; }"},
  };
  ParseProgram(test_files);
  EXPECT_ERRS("UnknownTypenameError(0:26-43)\n");
}

// TODO: tests for TypeSetImpl::Get.

// TODO: Test for wildcard import of package that doesn't exist. Also another
// one that doesn't have a corresponding type.

} // namespace types
