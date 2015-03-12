#include "types/types_test.h"

#include "joosc.h"

using ast::Program;
using base::ErrorList;
using base::FileSet;

namespace types {

  // Pairs of file name, file contents.
  sptr<const Program> ParseProgramWithStdlib(FileSet** fs, const vector<pair<string, string>>& file_contents, ErrorList* out) {
    // find third_party/cs444/stdlib/3.0 -type f -name '*.java'
    static const vector<string> stdlib = {
      "third_party/cs444/stdlib/3.0/java/io/Serializable.java",
      "third_party/cs444/stdlib/3.0/java/io/PrintStream.java",
      "third_party/cs444/stdlib/3.0/java/io/OutputStream.java",
      "third_party/cs444/stdlib/3.0/java/util/Arrays.java",
      "third_party/cs444/stdlib/3.0/java/lang/Byte.java",
      "third_party/cs444/stdlib/3.0/java/lang/Short.java",
      "third_party/cs444/stdlib/3.0/java/lang/Class.java",
      "third_party/cs444/stdlib/3.0/java/lang/Number.java",
      "third_party/cs444/stdlib/3.0/java/lang/Character.java",
      "third_party/cs444/stdlib/3.0/java/lang/Object.java",
      "third_party/cs444/stdlib/3.0/java/lang/Boolean.java",
      "third_party/cs444/stdlib/3.0/java/lang/Integer.java",
      "third_party/cs444/stdlib/3.0/java/lang/String.java",
      "third_party/cs444/stdlib/3.0/java/lang/Cloneable.java",
      "third_party/cs444/stdlib/3.0/java/lang/System.java"
    };

    base::FileSet::Builder fs_builder;
    for (const string& file_name : stdlib) {
      fs_builder.AddDiskFile(file_name);
    }
    for (auto contents : file_contents) {
      fs_builder.AddStringFile(contents.first, contents.second);
    }
    CHECK(fs_builder.Build(fs, out));

    return CompilerFrontend(CompilerStage::TYPE_CHECK, *fs, out);
  }

} // namespace types
